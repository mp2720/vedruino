#include "inc.h"

#if CONF_LIB_NETLOG_ENABLED

#include <esp_expression_with_stack.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>

#define SHSTACK_SIZE 8192
#define CTL_SERVER_TASK_STACK_SIZE 8192
#define CTL_SERVER_TASK_PRIORITY 10

#define CTL_SERVER_PORT 8419
#define CTL_SOCK_TIMEOUT_SECS 3
#define BCAST_BOOT_PORT 8419
#define BCAST_BOOT_NOTIFIES 5

#define SESSION_ID_SIZE 32
#define MAC_ADDR_SIZE 6

#define MAGIC "2720NETLOG"
#define MAGIC_SIZE (sizeof(MAGIC) - 1)
#define PROTO_SIZE 3
#define PROTO_UDP "udp"
#define PROTO_TCP "tcp"

static const char *TAG = "netlog";

static SemaphoreHandle_t clients_mutex, shstack_mutex;

// storage for FreeRTOS static data
static PK_NOINIT StaticSemaphore_t clients_mutex_st, shstack_mutex_st;
static PK_NOINIT StaticTask_t server_task_st;
static PK_NOINIT uint8_t server_task_stack[CTL_SERVER_TASK_STACK_SIZE];

#define CLIENTS_MUX_TAKE xSemaphoreTake(clients_mutex, portMAX_DELAY)
#define CLIENTS_MUX_GIVE xSemaphoreGive(clients_mutex)

static PK_NOINIT uint8_t mac_addr[MAC_ADDR_SIZE];
static PK_NOINIT uint8_t session_id[SESSION_ID_SIZE];

typedef struct pkNetlogClient {
    pkSocketHandle_t hd;
    char addr_str[PK_IP_ADDR_STR_LEN];
    struct {
        uint8_t flag : 1, pack_cnt : 7;
        pkIpAddr_t addr_port;
    } udp;
} pkNetlogClient_t;

// protected by clients_mutex
static int clients_num;
static int udp_clients_num;
static PK_NOINIT pkNetlogClient_t clients[CONF_LIB_NETLOG_MAX_CLIENTS];
// first byte for udp pack cnt
static PK_NOINIT char flush_with_shstack_udpbuf[1 + CONF_LIB_NETLOG_BUF_SIZE];
static PK_NOINIT const char *flush_with_shstack_buf_ptr;
static PK_NOINIT int flush_with_shstack_n;

// protected by shstack_mutex
static PK_NOINIT uint8_t shstack[SHSTACK_SIZE];

static PK_NOINIT FILE *netlogout;

typedef enum pkNetlogResp {
    // for udp
    PK_NETLOG_OK,
    // nothing to do, no extra log is required
    PK_NETLOG_OK_IGNORED,

    PK_NETLOG_ERR_INTERNAL,
    PK_NETLOG_ERR_MALFORMED_REQ,
    PK_NETLOG_ERR_CLIENTS_LIMIT,
    PK_NETLOG_ERR_TCP_CONNECT,
} pkNetlogResp_t;

#define RESP_STATUS_OK '+'
#define RESP_STATUS_ERR '-'

#define RESP_ERR_INTERNAL_MSG "internal error"
#define RESP_ERR_MALFORMED_REQ_MSG "malformed request"
#define RESP_ERR_CLIENTS_LIMIT_MSG "clients limit"
#define RESP_ERR_TCP_CONNECT_MSG "tcp connect error"

static void ctl_server_task(void *p);
static void bcast_boot_notify();
static pkNetlogResp_t ctl_handle_req(pkTcpClient_t chd, pkIpAddr_t addr);
static pkNetlogResp_t ctl_unsub(const char *log_addr_str);
static pkNetlogResp_t ctl_sub(const char *proto, pkIpAddr_t log_addr, const char *log_addr_str);
static int get_client_idx_by_addr_str(const char *addr_str);
static int get_free_idx_in_clients();
static void del_client(int i);
static int stdout_flush(void *cookie, const char *buf, int n);
static void flush_with_shstack(void);

void pk_netlog_init() {
    PK_ASSERT(esp_base_mac_addr_get(mac_addr) == ESP_OK);
    esp_fill_random(session_id, SESSION_ID_SIZE);

    clients_mutex = xSemaphoreCreateMutexStatic(&clients_mutex_st);
    PK_ASSERT(clients_mutex);
    shstack_mutex = xSemaphoreCreateMutexStatic(&shstack_mutex_st);
    PK_ASSERT(shstack_mutex);

    netlogout = fwopen(NULL, &stdout_flush);
    PK_ASSERT(netlogout);
    PK_ASSERT(setvbuf(netlogout, NULL, _IOLBF, CONF_LIB_NETLOG_BUF_SIZE) >= 0);

    stdout = netlogout;
    _GLOBAL_REENT->_stdout = netlogout;

    PK_ASSERT(xTaskCreateStatic(
        &ctl_server_task,
        "pknetlog_ctl",
        CTL_SERVER_TASK_STACK_SIZE,
        NULL,
        CTL_SERVER_TASK_PRIORITY,
        server_task_stack,
        &server_task_st
    ));
}

static void ctl_server_task(PK_UNUSED void *p) {
    PKLOGD("starting server");

    pkTcpServer_t shd = pk_tcp_server(CTL_SERVER_PORT, 1);
    PK_ASSERT(shd != PK_SOCKERR);

    PKLOGD("pk_tcp_server() done");
    bcast_boot_notify();

    PKLOGI("listening on port " PK_STRINGIZE(CTL_SERVER_PORT));

    while (1) {
        PKLOGD("server is ready to accept connections");
        pkIpAddr_t ctl_addr;
        pkTcpClient_t ctl_hd = pk_tcp_accept(shd, &ctl_addr);
        if (ctl_hd == PK_SOCKERR)
            goto err_cont;

        char cl_addr_str[PK_IP_ADDR_STR_LEN];
        pk_ip_addr2str(ctl_addr, cl_addr_str);

        PKLOGD("request from %s", cl_addr_str);

        uint8_t status;
        const char *err_msg;

        pkNetlogResp_t resp = ctl_handle_req(ctl_hd, ctl_addr);
        switch (resp) {
        case PK_NETLOG_ERR_INTERNAL:
            err_msg = RESP_ERR_INTERNAL_MSG;
            goto err_cont;
        case PK_NETLOG_ERR_MALFORMED_REQ:
            err_msg = RESP_ERR_MALFORMED_REQ_MSG;
            goto err_cont;
        case PK_NETLOG_ERR_CLIENTS_LIMIT:
            err_msg = RESP_ERR_CLIENTS_LIMIT_MSG;
            goto err_cont;
        case PK_NETLOG_ERR_TCP_CONNECT:
            err_msg = RESP_ERR_TCP_CONNECT_MSG;
            goto err_cont;

        case PK_NETLOG_OK:
            PKLOGI("request from %s processed successfully", cl_addr_str);
            PKLOGI("last reset reason: %s", pk_reset_reason_str());
            PKLOGI("%d connected clients:", clients_num);
            for (int i = 0; i < clients_num; ++i) {
                const char *proto_str = clients[i].udp.flag ? "udp" : "tcp";
                PKLOGI("%s %s", clients[i].addr_str, proto_str);
            }
        // fallthrough
        case PK_NETLOG_OK_IGNORED:
            status = RESP_STATUS_OK;
            pk_tcp_send(ctl_hd, &status, 1);
            pk_tcp_send(ctl_hd, mac_addr, MAC_ADDR_SIZE);
            pk_tcp_send(ctl_hd, session_id, SESSION_ID_SIZE);
            pk_sock_close(ctl_hd);
            continue;
        }

    err_cont:
        PKLOGE("failed to process request from %s: %s", cl_addr_str, err_msg);
        status = RESP_STATUS_ERR;
        pk_tcp_send(ctl_hd, &status, 1);
        size_t err_msg_len = strlen(err_msg);
        pk_tcp_send(ctl_hd, err_msg, err_msg_len);
        pk_sock_close(ctl_hd);
    }
}

static void bcast_boot_notify() {
    PKLOGD("bcast_boot_notify() started");
    uint8_t pack[MAGIC_SIZE + MAC_ADDR_SIZE + SESSION_ID_SIZE] = MAGIC;
    memcpy(pack + MAGIC_SIZE, mac_addr, MAC_ADDR_SIZE);
    memcpy(pack + MAGIC_SIZE + MAC_ADDR_SIZE, session_id, SESSION_ID_SIZE);

    PKLOGI("sending broadcast boot messages to port " PK_STRINGIZE(BCAST_BOOT_PORT));

    pkUdpClient_t chd = pk_udp_client();
    pkIpAddr_t br_addr = {.addr = PK_IP_BCAST_ADDR, .port = BCAST_BOOT_PORT};
    PK_ASSERT(chd != PK_SOCKERR);

    int ok_cnt = 0;
    for (int i = 0; i < BCAST_BOOT_NOTIFIES; ++i) {
        ssize_t sent = pk_udp_send(chd, br_addr, pack, sizeof pack);
        if (sent >= 0)
            ++ok_cnt;
    }

    PK_ASSERT(ok_cnt >= BCAST_BOOT_NOTIFIES / 3);

    PKLOGI(
        "%d out of " PK_STRINGIZE(BCAST_BOOT_NOTIFIES) " boot messages sent successfully",
        ok_cnt
    );

    pk_sock_close(chd);
}

static pkNetlogResp_t ctl_handle_req(pkTcpClient_t ctl_hd, pkIpAddr_t ctl_addr) {
    char magic[MAGIC_SIZE];
    if (!pk_tcp_recvn(ctl_hd, magic, MAGIC_SIZE) || memcmp(magic, MAGIC, MAGIC_SIZE) != 0)
        return PK_NETLOG_ERR_MALFORMED_REQ;

    char proto[PROTO_SIZE];
    if (!pk_tcp_recvn(ctl_hd, proto, PROTO_SIZE))
        return PK_NETLOG_ERR_MALFORMED_REQ;

    uint16_t port;
    if (!pk_tcp_recvn(ctl_hd, &port, 2))
        return PK_NETLOG_ERR_MALFORMED_REQ;
    port = ntohs(port);

    char op;
    if (!pk_tcp_recvn(ctl_hd, &op, 1))
        return PK_NETLOG_ERR_MALFORMED_REQ;

    pkIpAddr_t log_addr = {ctl_addr.addr, port};
    char log_addr_str[PK_IP_ADDR_STR_LEN];
    pk_ip_addr2str(log_addr, log_addr_str);

    if (op == 'S')
        return ctl_sub(proto, log_addr, log_addr_str);
    else if (op == 'U')
        return ctl_unsub(log_addr_str);
    else
        return PK_NETLOG_ERR_MALFORMED_REQ;
}

static pkNetlogResp_t ctl_sub(const char *proto, pkIpAddr_t log_addr, const char *log_addr_str) {
    pkSocketHandle_t log_chd;

    int i;

    CLIENTS_MUX_TAKE;
    i = get_client_idx_by_addr_str(log_addr_str);
    CLIENTS_MUX_GIVE;
    if (i >= 0) {
        PKLOGW("client is already sub, nothing to do");
        return PK_NETLOG_OK_IGNORED;
    }

    CLIENTS_MUX_TAKE;
    i = get_free_idx_in_clients();
    CLIENTS_MUX_GIVE;
    if (i < 0)
        return PK_NETLOG_ERR_CLIENTS_LIMIT;

    bool is_udp;
    if (memcmp(proto, PROTO_UDP, PROTO_SIZE) == 0) {
        is_udp = true;
        log_chd = pk_udp_client();
        if (log_chd == PK_SOCKERR)
            return PK_NETLOG_ERR_INTERNAL;
    } else if (memcmp(proto, PROTO_TCP, PROTO_SIZE) == 0) {
        is_udp = false;
        log_chd = pk_tcp_client();

        if (log_chd == PK_SOCKERR)
            return PK_NETLOG_ERR_INTERNAL;
        if (!pk_tcp_connect(log_chd, log_addr))
            return PK_NETLOG_ERR_TCP_CONNECT;
    } else
        return PK_NETLOG_ERR_MALFORMED_REQ;

    CLIENTS_MUX_TAKE;
    {
        // i may be changed because of del_client() call in other thread during stdout flush
        i = get_free_idx_in_clients();
        // should never fail
        PK_ASSERT(i >= 0);

        clients[i].hd = log_chd;
        clients[i].udp.flag = is_udp;
        strcpy(clients[i].addr_str, log_addr_str);
        if (is_udp) {
            clients[i].udp.pack_cnt = 0;
            clients[i].udp.addr_port = log_addr;
            ++udp_clients_num;
        }
        ++clients_num;
    }
    CLIENTS_MUX_GIVE;

    return PK_NETLOG_OK;
}

static pkNetlogResp_t ctl_unsub(const char *log_addr_str) {
    int i;

    CLIENTS_MUX_TAKE;
    {
        i = get_client_idx_by_addr_str(log_addr_str);
        if (i < 0) {
            CLIENTS_MUX_GIVE;
            PKLOGW("client is not sub, nothing to do");
            return PK_NETLOG_OK_IGNORED;
        }

        del_client(i);
    }
    CLIENTS_MUX_GIVE;

    return PK_NETLOG_OK;
}

// NOT thread safe. surround call with clients_mutex take/release
static int get_client_idx_by_addr_str(const char *addr_str) {
    for (int i = 0; i < clients_num; ++i)
        if (strcmp(addr_str, clients[i].addr_str) == 0)
            return i;

    return -1;
}

// NOT thread safe. surround call with clients_mutex take/release
static int get_free_idx_in_clients() {
    if (clients_num == CONF_LIB_NETLOG_MAX_CLIENTS)
        return -1;

    return clients_num;
}

// NOT thread safe. surround call with clients_mutex take/release
static void del_client(int i) {
    pk_sock_close(clients[i].hd);

    if (clients[i].udp.flag)
        --udp_clients_num;

    if (i != clients_num - 1)
        // overlap
        memmove(clients + i, clients + i + 1, (clients_num - i - 1) * sizeof(*clients));

    --clients_num;
}

static int stdout_flush(PK_UNUSED void *cookie, const char *buf, int n) {
    CLIENTS_MUX_TAKE;
    {
        /* fflush(pk_log_uartout); */
        flush_with_shstack_n = n;
        flush_with_shstack_buf_ptr = buf;
        esp_execute_shared_stack_function(
            shstack_mutex,
            shstack,
            SHSTACK_SIZE,
            &flush_with_shstack
        );
    }
    CLIENTS_MUX_GIVE;

    return n;
}

// NOT thread safe. surround call with clients_mutex take/release
static void flush_with_shstack(void) {
    // avoid recursion
    stdout = pk_log_uartout;

    /* esp_rom_printf("sending %d\n", flush_with_shstack_n); */

    size_t sent = 0;
    while (sent < (size_t)flush_with_shstack_n) {
        size_t cur_blk_size;
        if (udp_clients_num == 0) {
            cur_blk_size = flush_with_shstack_n - sent;
        } else {
            cur_blk_size = PK_MIN(flush_with_shstack_n - sent, CONF_LIB_NETLOG_BUF_SIZE);
            // no overlaps
            memcpy(flush_with_shstack_udpbuf + 1, flush_with_shstack_buf_ptr + sent, cur_blk_size);
        }

        for (int i = 0; i < clients_num; ++i) {
            ssize_t sent_blk_size;
            if (clients[i].udp.flag) {
                flush_with_shstack_udpbuf[0] = clients[i].udp.pack_cnt++;
                sent_blk_size = pk_udp_send(
                    clients[i].hd,
                    clients[i].udp.addr_port,
                    flush_with_shstack_udpbuf,
                    cur_blk_size + 1
                );
            } else {
                sent_blk_size =
                    pk_tcp_send(clients[i].hd, flush_with_shstack_buf_ptr + sent, cur_blk_size);
            }

            if (sent_blk_size < 0) {
                PKLOGE_UART("client %s will be disconnected after send error", clients[i].addr_str);
                del_client(i);
            }
        }

        // no subs or failed to send data to all subs
        if (clients_num == 0)
            write(0, flush_with_shstack_buf_ptr + sent, cur_blk_size);

        sent += cur_blk_size;
    }

    stdout = netlogout;
}

#endif // CONF_LIB_NETLOG_ENABLED
