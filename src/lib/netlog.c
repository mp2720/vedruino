#include "lib.h"

#include <esp_expression_with_stack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/reent.h>
#include <sys/socket.h>

#define MAX_CONNECTIONS 4
#define TCP_REQ_MAGIC "NETLOG"
#define TCP_RESP_FAILED "failed"
#define TCP_RESP_OK "ok"
#define TCP_RESP_NOT_SUB "notsub"
#define TCP_REQ_TYPE_SIZE 3
#define SERVER_TASK_STACK_SIZE 4096
#define SERVER_TASK_PRIORITY 1
#define SHARED_STACK_SIZE 4096
#define STDOUT_BUF_SIZE 128
// < 256
#define UDP_MAX_FAILS 10

static const char *TAG = "netlog";

typedef struct {
    pk_ip_hnd_t hnd;
    // для неактивного соединения первый байт должен быть равен 0.
    char addr_str[PK_IP_ADDRSTRLEN];
    uint8_t pack_cnt;
    uint8_t fails_cnt;
} conn_t;

static SemaphoreHandle_t conn_mutex, shared_stack_mutex;
static conn_t conns[MAX_CONNECTIONS];
static int conns_num;
static uint8_t shared_stack[SHARED_STACK_SIZE];
static char stdout_buf[STDOUT_BUF_SIZE];
static int udp_send_n;
static char *udp_send_buf;
static FILE *netlogout;

typedef enum { REQ_ERR, REQ_STATUS, REQ_SUB, REQ_UNSUB } req_type_t;

static void server_task(void *p);
static req_type_t proc_header(const pk_ip_hnd_t *hnd, uint16_t *out_port);
static bool req_sub(const pk_ip_hnd_t *hnd, uint16_t port, const char *ca_str);
static bool req_unsub(const pk_ip_hnd_t *hnd, uint16_t port);
static void req_status(const pk_ip_hnd_t *hnd, uint16_t port);
static int find_conn_idx(uint32_t addr, uint16_t port);
static void del_conn(int i);
static void dump_conns(void);

static int stdout_write(void *cookie, const char *buf, int n);
static void udp_send(void);

bool pk_netlog_init(void) {
    conn_mutex = xSemaphoreCreateMutex();
    if (conn_mutex == NULL) {
        PKLOGE("xSemaphoreCreateMutex() failed");
        return false;
    }
    shared_stack_mutex = xSemaphoreCreateMutex();
    if (shared_stack_mutex == NULL) {
        PKLOGE("xSemaphoreCreateMutex() failed");
        goto err;
    }

    netlogout = fwopen(NULL, &stdout_write);
    if (netlogout == NULL) {
        PKLOGE("fwopen() %s", strerror(errno));
        goto err;
    }
    // Первый байт для передачи номера пакета
    if (setvbuf(netlogout, stdout_buf + 1, _IOLBF, STDOUT_BUF_SIZE - 1) < 0) {
        PKLOGE("setvbuf() %s", strerror(errno));
        goto err;
    }

    stdout = netlogout;
    _GLOBAL_REENT->_stdout = netlogout;

    if (xTaskCreate(&server_task, "pk_netlog_serv", SERVER_TASK_STACK_SIZE, NULL,
                    SERVER_TASK_PRIORITY, NULL) != pdPASS) {
        PKLOGE("xTaskCreate() failed");
        goto err;
    }

    return true;

err:
    if (conn_mutex != NULL)
        vSemaphoreDelete(conn_mutex);

    if (shared_stack_mutex != NULL)
        vSemaphoreDelete(shared_stack_mutex);

    if (netlogout != NULL)
        fclose(netlogout);

    return false;
}

static void server_task(UNUSED void *p) {
    pk_ip_hnd_t tcp_hnd;
    if (!pk_tcp_server(&tcp_hnd, CONF_NETLOG_TCP_PORT, 2)) {
        PKLOGE("failed to start netlog tcp server");
        vTaskDelete(NULL);
    }

    PKLOGI("netlog listening on port %d", CONF_NETLOG_TCP_PORT);

    while (1) {
        if (!pk_tcp_accept(&tcp_hnd))
            goto err_cont;

        char ca_str[PK_IP_ADDRSTRLEN];
        if (!pk_ip_remote_addr2str(&tcp_hnd, ca_str))
            goto err_cont;

        uint16_t port;
        switch (proc_header(&tcp_hnd, &port)) {
        case REQ_STATUS:
            req_status(&tcp_hnd, port);
            // req_status() уже отправил ответ
            pk_ip_close_client(&tcp_hnd);
            continue;
        case REQ_SUB:
            PKLOGV("SUB req");
            if (!req_sub(&tcp_hnd, port, ca_str))
                goto err_cont;
            break;
        case REQ_UNSUB:
            PKLOGV("UNSUB req");
            if (!req_unsub(&tcp_hnd, port))
                goto err_cont;
            break;
        case REQ_ERR:
            goto err_cont;
        }

        pk_tcp_send(&tcp_hnd, TCP_RESP_OK, sizeof(TCP_RESP_OK) - 1);
        pk_ip_close_client(&tcp_hnd);
        PKLOGI("req ok");
        dump_conns();
        continue;

    err_cont:
        PKLOGE("failed processing tcp req from %s", ca_str);
        pk_tcp_send(&tcp_hnd, TCP_RESP_FAILED, sizeof(TCP_RESP_FAILED) - 1);
        pk_ip_close_client(&tcp_hnd);
    }
}

static req_type_t proc_header(const pk_ip_hnd_t *hnd, uint16_t *out_port) {
    char magic[sizeof(TCP_REQ_MAGIC) - 1];
    if (!pk_tcp_recvn(hnd, magic, sizeof magic) ||
        memcmp(magic, TCP_REQ_MAGIC, sizeof magic) != 0) {
        PKLOGE("cannot get valid magic from req");
        return REQ_ERR;
    }

    char req_type[3];
    if (!pk_tcp_recvn(hnd, req_type, sizeof req_type)) {
        PKLOGE("cannot get type from req");
        return REQ_ERR;
    }

    req_type_t t;
    if (memcmp(req_type, "STA", TCP_REQ_TYPE_SIZE) == 0)
        t = REQ_STATUS;
    else if (memcmp(req_type, "SUB", TCP_REQ_TYPE_SIZE) == 0)
        t = REQ_SUB;
    else if (memcmp(req_type, "UNS", TCP_REQ_TYPE_SIZE) == 0)
        t = REQ_UNSUB;
    else {
        PKLOGE("unknown request \"%.3s\"", req_type);
        return REQ_ERR;
    }

    if (!pk_tcp_recvn(hnd, out_port, sizeof *out_port)) {
        PKLOGE("cannot get port from req");
        return REQ_ERR;
    }

    return t;
}

static bool req_sub(const pk_ip_hnd_t *hnd, uint16_t port, const char *ca_str) {
    int free_idx = -1;
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (conns[i].hnd.remote.sin_addr.s_addr == hnd->remote.sin_addr.s_addr &&
            conns[i].hnd.remote.sin_port == port) {
            PKLOGW("client is already sub, nothing to do");
            return true;
        }

        if (conns[i].addr_str[0] == 0)
            free_idx = i;
    }

    if (free_idx < 0) {
        PKLOGE("reached max conns limit");
        return false;
    }

    conn_t *conn = &conns[free_idx];
    conn->pack_cnt = conn->fails_cnt = 0;
    if (!pk_udp_client(&conn->hnd))
        return false;

    pk_ip_set_remote(&conn->hnd, ntohl(hnd->remote.sin_addr.s_addr), ntohs(port));
    strcpy(conn->addr_str, ca_str);

    xSemaphoreTake(conn_mutex, portMAX_DELAY);
    ++conns_num;
    xSemaphoreGive(conn_mutex);

    return true;
}

static bool req_unsub(const pk_ip_hnd_t *hnd, uint16_t port) {
    int i = find_conn_idx(hnd->remote.sin_addr.s_addr, port);
    if (i < 0) {
        PKLOGW("client is not sub, nothing to do");
        return true;
    }

    xSemaphoreTake(conn_mutex, portMAX_DELAY);
    pk_udp_close_server(&conns[i].hnd);
    del_conn(i);
    xSemaphoreGive(conn_mutex);
}

static void req_status(const pk_ip_hnd_t *hnd, uint16_t port) {
    int i = find_conn_idx(hnd->remote.sin_addr.s_addr, port);
    if (i < 0)
        pk_tcp_send(hnd, TCP_RESP_NOT_SUB, sizeof(TCP_RESP_NOT_SUB) - 1);
    else
        pk_tcp_send(hnd, TCP_RESP_OK, sizeof(TCP_RESP_OK) - 1);
}

static int find_conn_idx(uint32_t addr, uint16_t port) {
    for (int i = 0; i < MAX_CONNECTIONS; ++i)
        if (conns[i].addr_str[0] != 0 && conns[i].hnd.remote.sin_addr.s_addr == addr &&
            conns[i].hnd.remote.sin_port == port)
            return i;

    return -1;
}

static void dump_conns(void) {
    PKLOGI("%d active conns:", conns_num);
    for (int i = 0; i < MAX_CONNECTIONS; ++i)
        if (conns[i].addr_str[0] != 0)
            PKLOGI("%s:%d", conns[i].addr_str, ntohs(conns[i].hnd.remote.sin_port));
}

static void del_conn(int i) {
    conns[i].addr_str[0] = 0;
    --conns_num;
}

static int stdout_write(UNUSED void *cookie, UNUSED const char *buf, int n) {
    xSemaphoreTake(conn_mutex, portMAX_DELAY);
    udp_send_n = n;
    udp_send_buf = stdout_buf;
    esp_execute_shared_stack_function(shared_stack_mutex, shared_stack, SHARED_STACK_SIZE,
                                      &udp_send);
    if (buf[n - 1] != '\n') {
        // Белые буквы на красном фоне, первый байт для номера пакета.
        char tear_msg[] = "\0\n\033[2;37;41mTEAR\033[2;39;49m\n";
        udp_send_buf = tear_msg;
        udp_send_n = sizeof(tear_msg) - 2;
        esp_execute_shared_stack_function(shared_stack_mutex, shared_stack, SHARED_STACK_SIZE,
                                          &udp_send);
    }
    xSemaphoreGive(conn_mutex);
    return n;
}

static void udp_send(void) {
    int failed_num = 0;
    for (int i = 0; i < conns_num; ++i) {
        if (conns[i].addr_str[0] == 0)
            continue;

        stdout_buf[0] = conns[i].pack_cnt++;
        // Чтобы при ошибке лог не привел к рекурсии
        // у каждой задачи свой stdout
        stdout = pk_log_uartout;
        bool send_status = pk_udp_send(&conns[i].hnd, stdout_buf, udp_send_n + 1);
        stdout = netlogout;

        if (!send_status) {
            if (conns[i].fails_cnt++ < UDP_MAX_FAILS) {
                PKLOGE("removing %s:%d from subs after %d fails to send logs", conns[i].addr_str,
                       ntohs(conns[i].hnd.remote.sin_port), UDP_MAX_FAILS);
                del_conn(i);
            }
            ++failed_num;
        } else {
            conns[i].fails_cnt = 0;
        }
    }

    if (failed_num == conns_num)
        write(0, stdout_buf + 1, udp_send_n);
}
