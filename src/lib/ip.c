#include "ip.h"

#include "inc.h"
#include <lwip/sockets.h>
#include <sys/socket.h>

#if CONF_IP_ENABLED

#define MAX_RETRIES_ON_ENOMEM 100

typedef enum {
    // retry MAX_RETRIES_ON_ENOMEM times
    SEND_RETRY,
    SEND_ERR,
    SEND_OK
} pkSendStatus_t;

static const char *TAG = "ip";

static struct sockaddr_in get_sockaddr(pkIpAddr_t addr) {
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(addr.port);
    saddr.sin_addr.s_addr = htonl(addr.addr);
    return saddr;
}

void pk_ip_addr2str(pkIpAddr_t addr, char str[PK_IP_ADDR_STR_LEN]) {
    // clang-format off
    uint8_t addr_bytes[] = {
        (addr.addr & 0xff000000) >> 24,
        (addr.addr & 0x00ff0000) >> 16,
        (addr.addr & 0x0000ff00) >> 8,
        (addr.addr & 0x000000ff)
    };
    // clang-format on

    snprintf(str, PK_IP_ADDR_STR_LEN, "%d.%d.%d.%d:%d", addr_bytes[0], addr_bytes[1], addr_bytes[2],
             addr_bytes[3], addr.port);
}

static pkSocketHandle_t create_srv_sock(uint16_t port, sa_family_t family) {
    pkSocketHandle_t srvhd = socket(AF_INET, family, 0);
    if (srvhd < 0) {
        PKLOGE("socket() %s", strerror(errno));
        return PK_SOCKERR;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(srvhd, (struct sockaddr *)&saddr, sizeof saddr) < 0) {
        PKLOGE("bind() %s", strerror(errno));
        return PK_SOCKERR;
    }

    return srvhd;
}

pkTcpServer_t pk_tcp_server(uint16_t port, int backlog) {
    pkSocketHandle_t srvhd = create_srv_sock(port, SOCK_STREAM);
    if (srvhd == PK_SOCKERR)
        return PK_SOCKERR;

    if (listen(srvhd, backlog) < 0) {
        PKLOGE("listen() %s", strerror(errno));
        return PK_SOCKERR;
    }

    return srvhd;
}

pkUdpHandle_t pk_udp_server(uint16_t port) {
    return create_srv_sock(port, SOCK_DGRAM);
}

static pkSocketHandle_t handle_client(int type) {
    pkSocketHandle_t clhd = socket(AF_INET, type, 0);
    if (clhd < 0) {
        PKLOGE("socket() %s", strerror(errno));
        return PK_SOCKERR;
    }
    return clhd;
}

pkTcpClient_t pk_tcp_client() {
    return handle_client(SOCK_STREAM);
}

pkUdpHandle_t pk_udp_client() {
    return handle_client(SOCK_DGRAM);
}

bool pk_tcp_connect(pkTcpClient_t chd, pkIpAddr_t srv_addr) {
    struct sockaddr_in saddr = get_sockaddr(srv_addr);
    if (connect(chd, (struct sockaddr *)&saddr, sizeof saddr) < 0) {
        PKLOGE("connect() %s", strerror(errno));
        return false;
    }
    return true;
}

pkTcpClient_t pk_tcp_accept(pkTcpServer_t shd, pkIpAddr_t *out_cl_addr) {
    struct sockaddr_in saddr;
    socklen_t saddr_len = sizeof saddr;
    pkSocketHandle_t chd = accept(shd, (struct sockaddr *)&saddr, &saddr_len);
    if (chd < 0) {
        PKLOGE("accept() %s", strerror(errno));
        return PK_SOCKERR;
    }

    if (out_cl_addr != NULL) {
        out_cl_addr->addr = ntohl(saddr.sin_addr.s_addr);
        out_cl_addr->port = ntohs(saddr.sin_port);
    }

    return chd;
}

static pkSendStatus_t handle_send(int *retries, ssize_t written) {
    if (written < 0) {
        if (errno == ENOMEM && (*retries)++ < MAX_RETRIES_ON_ENOMEM) {
            PKLOGW("send failed on ENOMEM, retrying...");
            return SEND_RETRY;
        } else {
            PKLOGE("send() %s", strerror(errno));
            return SEND_ERR;
        }
    }
    return SEND_OK;
}

ssize_t pk_tcp_send(pkTcpClient_t chd, const void *buf, size_t n) {
    int retries = 0;
    while (1) {
        ssize_t written = write(chd, buf, n);
        pkSendStatus_t st = handle_send(&retries, written);
        if (st == SEND_RETRY)
            continue;
        else
            return st == SEND_OK ? written : -1;
    }
}

ssize_t pk_udp_send(pkUdpHandle_t hd, pkIpAddr_t addr, const void *buf, size_t n) {
    struct sockaddr_in saddr = get_sockaddr(addr);
    int retries = 0;
    while (1) {
        ssize_t written = sendto(hd, buf, n, 0, (struct sockaddr *)&saddr, sizeof saddr);
        pkSendStatus_t st = handle_send(&retries, written);
        if (st == SEND_RETRY)
            continue;
        else
            return st == SEND_OK ? written : -1;
    }
}

static bool handle_n_func(const char *func_name, ssize_t processed, size_t exp) {
    if (processed != (ssize_t)exp) {
        PKLOGE("%s() processed %zd bytes, while %zd was expected", func_name, processed, exp);
        return false;
    }
    return true;
}

bool pk_tcp_sendn(pkTcpClient_t chd, const void *buf, size_t n) {
    ssize_t sent = pk_tcp_send(chd, buf, n);
    if (sent < 0)
        return false;
    return handle_n_func("pk_tcp_send", sent, n);
}

bool pk_udp_sendn(pkUdpHandle_t chd, pkIpAddr_t addr, const void *buf, size_t n) {
    ssize_t sent = pk_udp_send(chd, addr, buf, n);
    if (sent < 0)
        return false;
    return handle_n_func("pk_udp_send", sent, n);
}

ssize_t pk_tcp_recv(pkTcpClient_t chd, void *buf, size_t max_n) {
    ssize_t rd = read(chd, buf, max_n);
    if (rd < 0) {
        PKLOGE("read() %s", strerror(errno));
        return -1;
    }
    return rd;
}

ssize_t pk_udp_recv(pkUdpHandle_t hd, pkIpAddr_t addr, void *buf, size_t max_n) {
    struct sockaddr_in saddr = get_sockaddr(addr);
    socklen_t saddr_len = sizeof saddr;
    ssize_t rcv = recvfrom(hd, buf, max_n, 0, (struct sockaddr *)&saddr, &saddr_len);
    if (rcv < 0) {
        PKLOGE("recvfrom() %s", strerror(errno));
        return -1;
    }
    return rcv;
}

bool pk_tcp_recvn(pkTcpClient_t chd, void *buf, size_t n) {
    ssize_t rcv = pk_tcp_recv(chd, buf, n);
    if (rcv < 0)
        return false;
    return handle_n_func("pk_tcp_recv", rcv, n);
}

bool pk_udp_recvn(pkUdpHandle_t hd, pkIpAddr_t addr, void *buf, size_t n) {
    ssize_t rcv = pk_udp_recv(hd, addr, buf, n);
    if (rcv < 0)
        return false;
    return handle_n_func("pk_udp_recv", rcv, n);
}

bool pk_sock_close(pkSocketHandle_t hd) {
    int ret = close(hd);
    if (ret < 0) {
        PKLOGE("close() %s", strerror(errno));
        return false;
    }
    return true;
}

#endif // CONF_IP_ENABLED
