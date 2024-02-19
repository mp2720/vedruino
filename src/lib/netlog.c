#include "inc.h"

#if CONF_NETLOG_ENABLED
#include <assert.h>
#include <esp_attr.h>
#include <esp_expression_with_stack.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdbool.h>
#include <string.h>

#define SHSTACK_SIZE 4096
#define STDOUT_BUF_SIZE 256
#define MAX_CLIENTS 2
#define SERVER_TASK_STACK_SIZE 4096
#define SERVER_TASK_PRIORITY 10
#define SERVER_PORT 8419

static const char *TAG = "netlog";

static SemaphoreHandle_t clients_mutex, shstack_mutex;

static int clients_num;
static __NOINIT_ATTR struct {
    pkTcpClient_t hd;
    char addr_str[PK_IP_ADDR_STR_LEN];
} clients[MAX_CLIENTS];

static __NOINIT_ATTR uint8_t shstack[SHSTACK_SIZE];
static __NOINIT_ATTR char stdout_buf[STDOUT_BUF_SIZE];
static FILE *netlogout;
static __NOINIT_ATTR char *shstack_write_buf;
static __NOINIT_ATTR int shstack_write_n;

static void server_task(void *p);
static bool add_client(pkTcpClient_t chd, const char cl_addr_str[PK_IP_ADDR_STR_LEN]);
static void del_client(int i);
static int stdout_write(void *cookie, const char *buf, int n);
static void shstack_write(void);

bool pk_netlog_init() {
    clients_mutex = xSemaphoreCreateMutex();
    if (clients_mutex == NULL) {
        PKLOGE("xSemaphoreCreateMutex() failed");
        return false;
    }
    shstack_mutex = xSemaphoreCreateMutex();
    if (shstack_mutex == NULL) {
        PKLOGE("xSemaphoreCreateMutex() failed");
        goto err;
    }

    netlogout = fwopen(NULL, &stdout_write);
    if (netlogout == NULL) {
        PKLOGE("fwopen() %s", strerror(errno));
        goto err;
    }

    if (setvbuf(netlogout, stdout_buf, _IOLBF, STDOUT_BUF_SIZE) < 0) {
        PKLOGE("setvbuf() %s", strerror(errno));
        goto err;
    }

    stdout = netlogout;
    _GLOBAL_REENT->_stdout = netlogout;

    if (xTaskCreate(&server_task, "pk_netlog", SERVER_TASK_STACK_SIZE, NULL, SERVER_TASK_PRIORITY,
                    NULL) != pdPASS) {
        PKLOGE("xTaskCreate() failed");
        goto err;
    }

    return true;

err:
    if (clients_mutex != NULL)
        vSemaphoreDelete(clients_mutex);

    if (shstack_mutex != NULL)
        vSemaphoreDelete(shstack_mutex);

    if (netlogout != NULL)
        fclose(netlogout);

    return false;
}

static void server_task(UNUSED void *p) {
    pkTcpServer_t shd = pk_tcp_server(SERVER_PORT, 2);
    if (shd == PK_SOCKERR) {
        PKLOGE("netlog server task failed on init");
        vTaskDelete(NULL);
    }

    PKLOGI("listening on port %d", SERVER_PORT);

    while (1) {
        pkIpAddr_t cl_addr;
        pkTcpClient_t chd = pk_tcp_accept(shd, &cl_addr);
        if (chd == PK_SOCKERR)
            goto err_cont;

        char cl_addr_str[PK_IP_ADDR_STR_LEN];
        pk_ip_addr2str(cl_addr, cl_addr_str);

        bool res = add_client(chd, cl_addr_str);
        if (!res)
            goto err_cont;

        PKLOGI("added client %s", cl_addr_str);
        PKLOGI("last reset reason: %s", pk_reset_reason_str());
        PKLOGI("%d connected clients:", clients_num);
        for (int i = 0; i < clients_num; ++i)
            PKLOGI("%s", clients[i].addr_str);
        continue;

    err_cont:
        PKLOGE("failed to add client %s", cl_addr_str);
        pk_sock_close(chd);
    }
}

static bool add_client(pkTcpClient_t chd, const char cl_addr_str[PK_IP_ADDR_STR_LEN]) {
    if (clients_num == MAX_CLIENTS) {
        const char msg[] = "reached max clients limit";
        PKLOGE("%s", msg);
        pk_tcp_send(chd, msg, sizeof(msg) - 1);
        return false;
    }

    assert(xSemaphoreTake(clients_mutex, portMAX_DELAY));
    clients[clients_num].hd = chd;
    memcpy(clients[clients_num].addr_str, cl_addr_str, PK_IP_ADDR_STR_LEN);
    assert(xSemaphoreGive(clients_mutex));

    ++clients_num;

    return true;
}

static int stdout_write(UNUSED void *cookie, UNUSED const char *buf, int n) {
    stdout = pk_log_uartout;

    xSemaphoreTake(clients_mutex, portMAX_DELAY);
    shstack_write_n = n;
    shstack_write_buf = stdout_buf;

    esp_execute_shared_stack_function(shstack_mutex, shstack, SHSTACK_SIZE, &shstack_write);
    if (buf[n - 1] != '\n') {
        // Белые буквы на красном фоне
        char tear_msg[] = "\n\033[2;37;41mTEAR\033[2;39;49m\n";
        shstack_write_buf = tear_msg;
        shstack_write_n = sizeof(tear_msg) - 2;
        esp_execute_shared_stack_function(shstack_mutex, shstack, SHSTACK_SIZE, &shstack_write);
    }

    xSemaphoreGive(clients_mutex);
    stdout = netlogout;
    return n;
}

static void shstack_write(void) {
    for (int i = 0; i < clients_num; ++i)
        if (pk_tcp_send(clients[i].hd, shstack_write_buf, shstack_write_n) < 0) {
            PKLOGE_UART("client %s will be disconnected due to error while sending message",
                        clients[i].addr_str);
            del_client(i);
        }

    if (clients_num == 0)
        write(0, shstack_write_buf, shstack_write_n);
}

static void del_client(int i) {
    pk_sock_close(clients[i].hd);

    if (i != clients_num - 1)
        // overlap
        memmove(clients + i, clients + i + 1, (clients_num - i - 1) * sizeof(*clients));

    --clients_num;
}

#endif // CONF_NETLOG_ENABLED
