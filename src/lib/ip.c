#include "net.h"

#include "lib.h"
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>

#if CONF_IP_ENABLED

#define RETRIES_ON_ENOMEM 100

typedef enum {
    // retry RETRIES_ON_ENOMEM times
    SEND_RETRY,
    SEND_ERR,
    SEND_OK
} send_status_t;

static const char *TAG = "ip";

bool pk_ip_remote_addr2str(const pk_ip_hnd_t *hnd, char str[PK_IP_ADDRSTRLEN]) {
    if (inet_ntop(AF_INET, &hnd->remote.sin_addr, str, PK_IP_ADDRSTRLEN) == NULL) {
        PKLOGE("inet_ntop() %s", strerror(errno));
        return false;
    }

    return true;
}

bool pk_ip_cmp_addr_port(const struct sockaddr_in *addr1, const struct sockaddr_in *addr2) {
    return addr1->sin_addr.s_addr == addr2->sin_addr.s_addr && addr1->sin_port == addr2->sin_port;
}

static bool handle_server(pk_ip_hnd_t *hnd, uint16_t port, int backlog, int type) {
    hnd->ssfd = socket(AF_INET, type, 0);
    if (hnd->ssfd < 0) {
        PKLOGE("socket() %s", strerror(errno));
        return false;
    }

    memset(&hnd->local, 0, sizeof hnd->local);
    memset(&hnd->remote, 0, sizeof hnd->remote);
    hnd->local.sin_family = AF_INET;
    hnd->local.sin_port = htons(port);
    hnd->local.sin_addr.s_addr = INADDR_ANY;

    if (bind(hnd->ssfd, (struct sockaddr *)&hnd->local, sizeof(hnd->local)) < 0) {
        PKLOGE("bind() %s", strerror(errno));
        return false;
    }

    if (type == SOCK_STREAM) {
        if (listen(hnd->ssfd, backlog) < 0) {
            PKLOGE("listen() %s", strerror(errno));
            return false;
        }
    }

    return true;
}

bool pk_tcp_server(pk_ip_hnd_t *hnd, uint16_t port, int backlog) {
    return handle_server(hnd, port, backlog, SOCK_STREAM);
}

bool pk_udp_server(pk_ip_hnd_t *hnd, uint16_t port, int backlog) {
    return handle_server(hnd, port, backlog, SOCK_DGRAM);
}

static bool handle_client(pk_ip_hnd_t *hnd, int type) {
    memset(&hnd->remote, 0, sizeof hnd->remote);
    hnd->csfd = socket(AF_INET, type, 0);
    if (hnd->csfd < 0) {
        PKLOGE("socket() %s", strerror(errno));
        return false;
    }
    return true;
}

bool pk_tcp_client(pk_ip_hnd_t *hnd) {
    return handle_client(hnd, SOCK_STREAM);
}

bool pk_udp_client(pk_ip_hnd_t *hnd) {
    return handle_client(hnd, SOCK_DGRAM);
}

void pk_ip_set_remote(pk_ip_hnd_t *hnd, uint32_t addr, uint16_t port) {
    hnd->remote.sin_family = AF_INET;
    hnd->remote.sin_addr.s_addr = htonl(addr);
    hnd->remote.sin_port = htons(port);
}

bool pk_ip_set_remote_by_hostname(pk_ip_hnd_t *hnd, const char *hostname, uint16_t port) {
    // TODO: сделать так, чтобы работало с mdns.
    return false;
}

bool pk_tcp_accept(pk_ip_hnd_t *hnd) {
    socklen_t len = sizeof hnd->remote;
    hnd->csfd = accept(hnd->ssfd, (struct sockaddr *)&hnd->remote, &len);
    if (hnd->csfd < 0) {
        PKLOGE("accept() %s", strerror(errno));
        return false;
    }

    return true;
}

static send_status_t handle_send(int *retries, size_t exp, int n) {
    if (n < 0) {
        if (errno == ENOMEM && (*retries)++ < RETRIES_ON_ENOMEM) {
            PKLOGW("send failed on ENOMEM, retrying...");
            return SEND_RETRY;
        } else {
            PKLOGE("send failed: %s", strerror(errno));
            return SEND_ERR;
        }
    }

    if ((size_t)n != exp) {
        PKLOGE("only %zu bytes written, while %zu was expected", n, exp);
        return SEND_ERR;
    }

    return SEND_OK;
}

bool pk_tcp_send(const pk_ip_hnd_t *hnd, const void *buf, size_t n) {
    int retries = 0;
    while (1) {
        int written = write(hnd->csfd, buf, n);
        send_status_t st = handle_send(&retries, n, written);
        if (st == SEND_RETRY)
            continue;
        else
            return st == SEND_OK;
    }
}

bool pk_udp_send(pk_ip_hnd_t *hnd, const void *buf, size_t n) {
    int retries = 0;
    while (1) {
        int written =
            sendto(hnd->csfd, buf, n, 0, (struct sockaddr *)&hnd->remote, sizeof hnd->remote);
        send_status_t st = handle_send(&retries, n, written);
        if (st == SEND_RETRY)
            continue;
        else
            return st == SEND_OK;
    }
}

size_t pk_tcp_recv(const pk_ip_hnd_t *hnd, void *buf, size_t max_n) {
    int rcv = read(hnd->csfd, buf, max_n);
    if (rcv < 0) {
        PKLOGE("read() %s", strerror(errno));
        return -1;
    }

    return rcv;
}

size_t pk_udp_recv(pk_ip_hnd_t *hnd, void *buf, size_t max_n) {
    socklen_t addr_len;
    int rcv = recvfrom(hnd->csfd, buf, max_n, 0, (struct sockaddr *)&hnd->remote, &addr_len);
    if (rcv < 0) {
        PKLOGE("read() %s", strerror(errno));
        return -1;
    }
    return rcv;
}

static bool handle_recvn(int rcv, size_t n) {
    if (rcv < 0) {
        PKLOGE("read() %s", strerror(errno));
        return false;
    }

    if ((size_t)rcv != n) {
        PKLOGE("expected %zu bytes, but got %zu", n, rcv);
        return false;
    }

    return true;
}

bool pk_tcp_recvn(const pk_ip_hnd_t *hnd, void *buf, size_t n) {
    int rcv = read(hnd->csfd, buf, n);
    return handle_recvn(rcv, n);
}

bool pk_udp_recvn(pk_ip_hnd_t *hnd, void *buf, size_t n) {
    socklen_t addr_len;
    int rcv = recvfrom(hnd->csfd, buf, n, 0, (struct sockaddr *)&hnd->remote, &addr_len);
    return handle_recvn(rcv, n);
}

bool pk_ip_close_client(pk_ip_hnd_t *hnd) {
    if (close(hnd->csfd) < 0) {
        PKLOGE("close() %s", strerror(errno));
        return false;
    }
    return true;
}

bool pk_tcp_close_server(pk_ip_hnd_t *hnd) {
    if (close(hnd->ssfd) < 0) {
        PKLOGE("close() %s", strerror(errno));
        return false;
    }
    return true;
}

#endif // CONF_IP_ENABLED
