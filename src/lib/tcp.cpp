#include "lwip/netdb.h"
#include "sdkconfig.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netdb.h>
#include "esp_log.h"

#include "tcp.h"
#include "log.h"

static const char * TAG = "TCP";

int log_socket = -1;

int tcp_connect(const char * host_name, const char * host_port) {
    int result_socket = -1;

    struct addrinfo *servinfo;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int res = getaddrinfo(host_name, host_port, &hints, &servinfo);
    if (res != 0) {
        ESP_LOGE(TAG, "getaddrinfo() error");
        return -1;
    }
    struct addrinfo dest_servinfo = *servinfo;
    struct sockaddr dest_addr = *(servinfo->ai_addr);
    freeaddrinfo(servinfo);

    result_socket = socket(dest_servinfo.ai_family, dest_servinfo.ai_socktype, dest_servinfo.ai_protocol);
    if (result_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d - %s", errno, strerror(errno));
        return -1;
    } else {
        ESP_LOGD(TAG, "Socket successfully created: %d", result_socket);
    }

    ESP_LOGD(TAG, "Connecting to %s:%s", host_name, host_port);
    int err = connect(result_socket, &dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Error when connecting a socket: %d - %s", errno, strerror(errno));
        return -1;
    }
    ESP_LOGI(TAG, "Successfully connected");
    return result_socket;
}

int tcp_send(int socket, const char * payload, size_t len) {
    if (!len) len = strlen(payload);
    int err = send(socket, payload, len, 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d - %s", errno, strerror(errno));
        return 1;
    }
    return 0;
}

static int tcp_vprintf(int socket, const char * format, va_list vargs) {
    const int BUFF_SIZE = 256;
    char buff[BUFF_SIZE];
    int bytes = vsnprintf(buff, BUFF_SIZE-1, format, vargs);
    if(bytes<BUFF_SIZE) { 
        tcp_send(socket, buff, bytes);
    } else {
        char * larger_buff = (char*)malloc(bytes+1);
        if (!larger_buff) {
            ESP_LOGE(TAG, "malloc error, no memory");
            return -1;
        }
        vsprintf(larger_buff, format, vargs);
        tcp_send(socket, larger_buff, bytes);
        free(larger_buff);
    }
    return bytes;
}

int tcp_printf(int socket, const char * format, ...) {
    va_list args;
    va_start(args, format);
    int res = tcp_vprintf(socket, format, args);
    va_end(args);
    return res;
}

int tcp_log_printf(const char * format, ...) {
    printf_like_t last = log_output;
    log_output = printf;
    va_list args;
    va_start(args, format);
    int res = tcp_vprintf(log_socket, format, args);
    va_end(args);
    log_output = last;
    return res;
}

int tcp_close(int socket) {
    if(socket < 0) {
        ESP_LOGW(TAG, "Nothing to close");
        return 0;
    }
    if (close(socket)) {
        ESP_LOGE(TAG, "Unable to close socket: errno %d - %s", errno, strerror(errno));
        return 1;
    } else {
        ESP_LOGV(TAG, "Socket %d successfully closed", socket);
        return 0; 
    }
}
