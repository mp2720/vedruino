#include "lib.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define PACKS_NUM 10000
#define PACK_SIZE 512
#define PORT 1498
#define HOST "192.168.43.5"

static const char *TAG = "udp_test";

static char buf[PACK_SIZE] = {};

void udp_test() {
    int sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sfd < 0) {
        ESP_LOGE(TAG, "socket() failed");
        return;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (!inet_aton(HOST, &servaddr.sin_addr.s_addr)) {
        ESP_LOGE(TAG, "inet_aton() failed");
        return;
    }

    int64_t t1 = esp_timer_get_time(), t2;

    for (int i = 0; i < PACKS_NUM; ++i) {
    retry:
        sprintf(buf, "%.9d", i);
        ssize_t ret =
            sendto(sfd, buf, sizeof buf, 0, (struct sockaddr *)&servaddr, sizeof servaddr);
        if (ret < 0) {
            ESP_LOGE(TAG, "sendto failed %s, retrying", strerror(errno));
            if (errno == ENOMEM)
                goto retry;
        }
        ESP_LOGI(TAG, "%d", i);
    }

    t2 = esp_timer_get_time();

    float dt_secs = (t2 - t1) / 1e6f;

    const size_t sent_size = PACKS_NUM * PACK_SIZE;

    ESP_LOGI(TAG, "sent %zu bytes in %f secs (%f b/s)", sent_size, dt_secs, sent_size / dt_secs,
             PACKS_NUM);
}
