/*
 * Тот миша, который сегодня, слишком жирный и будет медленно таскать ноутбуки к платам.
 * Поэтому я его опять автоматизировал.
 *
 * TODO: попробовать в две задачи работать (одна получает данные, другая пишет и хэш считает).
 * TODO: попробовать сжатие (хотя бы LZW).
 * TODO: разобраться как выключать остальные задачи во время прошивки, чтобы время зря не тратили.
 */

#include "lib.h"

#include <errno.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <math.h>
#include <mbedtls/md5.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *TAG = "OTA";

#define SERVER_TASK_STACK_SIZE 4096
#define FIRMWARE_BUF_SIZE 512

#define MAGIC "TCPOTAUPDATE"
#define MAGIC_SIZE (sizeof(MAGIC) - 1)
#define MD5_SIZE 16
#define MD5STR_SIZE (MD5_SIZE * 2 + 1)

#define OK_TCP_RESPONSE "ok"
#define FAILED_TCP_RESPONSE "failed"

/*
 * запрос должен выглядеть так: TCPOTAUDPATE<md5><size><data>
 * md5 и size считаются для data
 */
static struct {
    char magic[MAGIC_SIZE];
    uint8_t md5[MD5_SIZE];
    /* null-terminated */
    char md5str[MD5STR_SIZE];
    size_t size;
} header;

static int ssfd, port, csfd;

static void server_task(void *);
static inline int read_header();
static inline int update();
static char nibble_str(uint8_t n);
static inline void md5str(const uint8_t md5[MD5_SIZE], char out_str[MD5STR_SIZE]);

int ota_server_start(int _port) {
    port = _port;

    ssfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ssfd < 0) {
        DFLT_LOGE(TAG, "socket() error: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(ssfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        DFLT_LOGE(TAG, "bind() failed: %s", strerror(errno));
        return -1;
    }

    if (listen(ssfd, 1) < 0) {
        DFLT_LOGE(TAG, "listen() failed: %s", strerror(errno));
        return -1;
    }

    if (xTaskCreate(&server_task, "ota_tcp_server", SERVER_TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY,
                    NULL) != pdPASS) {
        DFLT_LOGE(TAG, "xTaskCreate() failed");
        return -1;
    }

    return 0;
}

static void server_task(UNUSED void *p) {
    struct sockaddr_in ca;
    socklen_t caddr_len = sizeof ca;
    char ca_str[INET_ADDRSTRLEN];
    while (1) {
        DFLT_LOGI(TAG, "listening on port %d", port);

        csfd = accept(ssfd, (struct sockaddr *)&ca, &caddr_len);
        int64_t start_time = esp_timer_get_time();
        if (csfd < 0) {
            DFLT_LOGE(TAG, "accept() failed: %s", strerror(errno));
            goto error_cont;
        }

        if (inet_ntop(AF_INET, &(ca.sin_addr), ca_str, INET_ADDRSTRLEN) == NULL) {
            DFLT_LOGE(TAG, "inet_ntop() failed: %s", strerror(errno));
            goto error_cont;
        }

        DFLT_LOGI(TAG, "update request from %s:%d", ca_str, ntohl(ca.sin_port));

        if (read_header() < 0) {
            DFLT_LOGW(TAG, "request has invalid header");
            goto error_cont;
        }

        DFLT_LOGI(TAG, "size of new firmware: %zu, md5: %s", header.size, header.md5str);
        DFLT_LOGI(TAG, "retreiving firmware...");
        if (update() < 0) {
            DFLT_LOGE(TAG, "failed to update, staying on the old version");
            goto error_cont;
        }

        float secs = (float)(esp_timer_get_time() - start_time) / 1e6f;
        DFLT_LOGI(TAG, "updated in %f secs", secs);

        write(csfd, OK_TCP_RESPONSE, sizeof(OK_TCP_RESPONSE) - 1);
        close(csfd);

        DFLT_LOGI(TAG, "new firmware is loaded, rebooting...");
        esp_restart();

    error_cont:
        write(csfd, FAILED_TCP_RESPONSE, sizeof(FAILED_TCP_RESPONSE) - 1);
        close(csfd);
    }
}

static inline int read_header() {
    uint32_t size;

    if (read(csfd, header.magic, MAGIC_SIZE) != MAGIC_SIZE ||
        read(csfd, header.md5, MD5_SIZE) != MD5_SIZE ||
        read(csfd, &size, sizeof(size)) != sizeof(size))
        return -1;

    if (memcmp(header.magic, MAGIC, MAGIC_SIZE) != 0)
        return -1;

    header.size = (size_t)ntohl(size);
    md5str(header.md5, header.md5str);
    return 0;
}

static inline int update() {
    esp_err_t err;
    esp_ota_handle_t ota_handle;
    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);

    err = esp_ota_begin(part, header.size, &ota_handle);
    if (err != ESP_OK) {
        DFLT_LOGE(TAG, "esp_ota_begin() failed: %s", esp_err_to_name(err));
        return -1;
    }

    DFLT_LOGI(TAG, "writing firmware to %s", part->label);

    int64_t start_time = esp_timer_get_time();

    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    /* По документации mbedtls эта функция должна вызываться, но с ней линкер выдает
     * undefined reference. Без нее все работает. */
    /* mbedtls_md5_starts(&md5_ctx); */

    uint8_t buf[FIRMWARE_BUF_SIZE];
    size_t loaded = 0;
    while (loaded < header.size) {
        int n = read(csfd, buf, FIRMWARE_BUF_SIZE);
        if (n < 0) {
            DFLT_LOGE(TAG, "read() failed: %s", strerror(errno));
            goto err_abort_ota;
        }

        if (n == 0) {
            DFLT_LOGE(TAG, "expected %zu bytes of firmware, got %zu", header.size, loaded);
            goto err_abort_ota;
        }

        loaded += n;
        mbedtls_md5_update(&md5_ctx, buf, n);

        err = esp_ota_write(ota_handle, buf, n);
        if (err != ESP_OK) {
            DFLT_LOGE(TAG, "esp_ota_write() failed: %s", esp_err_to_name(err));
            goto err_abort_ota;
        }
    }

    uint8_t computed_md5[MD5_SIZE];
    mbedtls_md5_finish(&md5_ctx, computed_md5);

    if (memcmp(header.md5, computed_md5, MD5_SIZE) != 0) {
        char computed_md5str[MD5STR_SIZE];
        md5str(computed_md5, computed_md5str);
        DFLT_LOGE(TAG, "md5 differs %s (given) != %s (computed)", header.md5str, computed_md5str);
        goto err_abort_ota;
    }

    float secs = (float)(esp_timer_get_time() - start_time) / 1e6f, avg_speed;
    if (secs == 0)
        /* невозможно */
        avg_speed = INFINITY;
    else
        avg_speed = (float)header.size / secs;

    DFLT_LOGI(TAG, "loaded %zu bytes in %f secs with avg speed %f b/s", header.size, secs,
             avg_speed);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        DFLT_LOGE(TAG, "esp_ota_end() failed: %s", esp_err_to_name(err));
        goto err_abort_ota;
    }

    mbedtls_md5_free(&md5_ctx);

    err = esp_ota_set_boot_partition(part);
    if (err != ESP_OK) {
        DFLT_LOGE(TAG, "esp_ota_set_boot_partition() failed: %s", esp_err_to_name(err));
        return -1;
    }

    return 0;

err_abort_ota:
    esp_ota_abort(ota_handle);
    mbedtls_md5_free(&md5_ctx);
    return -1;
}

static void md5str(const uint8_t md5[MD5_SIZE], char out_str[MD5STR_SIZE]) {
    for (int i = 0; i < MD5_SIZE; ++i) {
        uint8_t low = md5[i] & 0x0F;
        uint8_t high = (md5[i] & 0xF0) << 4;
        out_str[2 * i] = nibble_str(high);
        out_str[2 * i + 1] = nibble_str(low);
    }
    out_str[MD5STR_SIZE - 1] = 0;
}

static inline char nibble_str(uint8_t n) {
    if (n <= 9)
        return '0' + n;
    else
        return 'A' + n - 10;
}
