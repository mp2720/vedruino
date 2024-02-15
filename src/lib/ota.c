/*
 * Тот миша, который сегодня, слишком жирный и будет медленно таскать ноутбуки к платам.
 * Поэтому я его опять автоматизировал.
 */

#include "lib.h"

#include <errno.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#if CONF_TCP_OTA_VERIFY_MD5
#include <mbedtls/md5.h>
#endif // CONF_TCP_OTA_VERIFY_MD5

static const char *TAG = "ota";

#define SERVER_TASK_STACK_SIZE 4096
#define SERVER_TASK_LOW_PRIORITY 1
// Приоритет выше чем у wifi ставить смысла нет.
#define SERVER_TASK_HIGH_PRIORITY 23
#define FIRMWARE_BUF_SIZE 4096

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

typedef enum {
    // Ошибка проверки раздела.
    PART_STATUS_ERROR,
    // Раздел чист.
    PART_STATUS_ERASED,
    // Чисти-чисти раздел.
    PART_STATUS_NEED_ERASE
} upd_part_status_t;

static int ssfd, csfd;
static const esp_partition_t *upd_part;
static bool part_clean;

static void server_task(void *);
static bool init_socket();
static inline bool prepare_part();
static inline upd_part_status_t check_upd_part();
static inline bool read_header();
static inline bool update();
#if CONF_TCP_OTA_VERIFY_MD5
static inline void md5str(const uint8_t md5[MD5_SIZE], char out_str[MD5STR_SIZE]);
static char nibble_str(uint8_t n);
#endif // CONF_TCP_OTA_VERIFY_MD5

bool ota_server_start() {
    if (xTaskCreate(&server_task, "ota_tcp_server", SERVER_TASK_STACK_SIZE, NULL,
                    SERVER_TASK_LOW_PRIORITY, NULL) != pdPASS) {
        NLOGE("xTaskCreate failed");
        return false;
    }
    return true;
}

static void server_task(UNUSED void *p) {
    if (!init_socket())
        goto fatal_err;

    if (!prepare_part()) {
        NLOGE("part is not prepared, cannot continue");
        goto fatal_err;
    }

    part_clean = true;

    struct sockaddr_in ca;
    socklen_t caddr_len = sizeof ca;
    char ca_str[INET_ADDRSTRLEN];
    while (1) {
        NLOGI("listening on port %d", CONF_OTA_PORT);

        csfd = accept(ssfd, (struct sockaddr *)&ca, &caddr_len);
        int64_t start_time = esp_timer_get_time();
        if (csfd < 0) {
            NLOGE("accept() %s", strerror(errno));
            goto err_cont;
        }

        vTaskPrioritySet(NULL, SERVER_TASK_HIGH_PRIORITY);
        sysmon_pause();

        if (inet_ntop(AF_INET, &(ca.sin_addr), ca_str, INET_ADDRSTRLEN) == NULL) {
            NLOGE("inet_ntop() %s", strerror(errno));
            goto err_cont;
        }

        NLOGI("update request from %s", ca_str);

        if (!read_header()) {
            NLOGW("request has invalid header");
            goto err_cont;
        }

#if CONF_TCP_OTA_VERIFY_MD5
        NLOGI("size of new firmware: %zu, md5: %s", header.size, header.md5str);
#else
        NLOGI("size of new firmware: %zu, md5 is ignored", header.size);
#endif // CONF_TCP_OTA_VERIFY_MD5

        NLOGI("retreiving firmware...");
        if (!update()) {
            NLOGE("failed to update, staying on the old version");
            goto err_cont;
        }

        float secs = (float)(esp_timer_get_time() - start_time) / 1e6f;
        NLOGI("updated in %f secs", secs);

        write(csfd, OK_TCP_RESPONSE, sizeof(OK_TCP_RESPONSE) - 1);
        close(csfd);

        NLOGI("new firmware is loaded, rebooting...");

        // Время на запись ответа в сокет.
        vTaskDelay(80 / portTICK_RATE_MS);
        esp_restart();

        // Произошла ошибка, но сервер может продолжить работать.
    err_cont:
        write(csfd, FAILED_TCP_RESPONSE, sizeof(FAILED_TCP_RESPONSE) - 1);
        close(csfd);

        if (!part_clean) {
            NLOGV("part is not clean, erasing...");

            esp_err_t err = esp_partition_erase_range(upd_part, 0, upd_part->size);
            if (err != ESP_OK) {
                NLOGE("esp_partition_erase_range %s", esp_err_to_name(err));
                goto fatal_err;
            }
        }

        sysmon_resume();
        vTaskPrioritySet(NULL, SERVER_TASK_LOW_PRIORITY);

        part_clean = true;
    }

fatal_err:
    NLOGE("fatal error, shutting down the server");
    close(csfd);
    vTaskDelete(NULL);
}

static inline bool init_socket() {
    ssfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ssfd < 0) {
        NLOGE("socket() %s", strerror(errno));
        return false;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(CONF_OTA_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(ssfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        NLOGE("bind() %s", strerror(errno));
        goto err;
    }

    if (listen(ssfd, 1) < 0) {
        NLOGE("listen() %s", strerror(errno));
        goto err;
    }

    return true;

err:
    close(ssfd);
    return false;
}

static inline bool prepare_part() {
    if ((upd_part = esp_ota_get_next_update_partition(NULL)) == NULL) {
        NLOGE("no ota part found");
        return false;
    }

    NLOGV("%s is next update part", upd_part->label);

    upd_part_status_t status = check_upd_part();
    if (status == PART_STATUS_ERROR)
        return false;

    if (status == PART_STATUS_NEED_ERASE) {
        esp_err_t err = esp_partition_erase_range(upd_part, 0, upd_part->size);
        if (err != ESP_OK) {
            NLOGE("esp_partition_erase_range %s", esp_err_to_name(err));
            return false;
        }

        NLOGV("part %s erased", upd_part->label);
    } else {
        NLOGV("part %s is clean", upd_part->label);
    }

    return true;
}

static inline upd_part_status_t check_upd_part() {
    const size_t BLK_SIZE = 4096;

    uint8_t *buf = malloc(BLK_SIZE);
    if (buf == NULL) {
        NLOGE("malloc failed");
        return PART_STATUS_ERROR;
    }

    bool erased = true;
    for (size_t offs = 0; offs < upd_part->size; offs += BLK_SIZE) {
        esp_err_t err = esp_partition_read(upd_part, offs, buf, BLK_SIZE);
        if (err != ESP_OK) {
            NLOGE("esp_partition_read %s", esp_err_to_name(err));
            free(buf);
            return PART_STATUS_ERROR;
        }

        bool erased_blk = true;
        for (size_t i = 0; i < 4096; ++i)
            erased_blk = erased_blk && buf[i] == 0xff;

        erased = erased && erased_blk;
    }

    free(buf);

    return erased ? PART_STATUS_ERASED : PART_STATUS_NEED_ERASE;
}

static inline bool read_header() {
    uint32_t size;

    if (read(csfd, header.magic, MAGIC_SIZE) != MAGIC_SIZE ||
        read(csfd, header.md5, MD5_SIZE) != MD5_SIZE ||
        read(csfd, &size, sizeof(size)) != sizeof(size))
        return false;

    if (memcmp(header.magic, MAGIC, MAGIC_SIZE) != 0)
        return false;

    header.size = (size_t)ntohl(size);
#if CONF_TCP_OTA_VERIFY_MD5
    md5str(header.md5, header.md5str);
#endif // CONF_TCP_OTA_VERIFY_MD5
    return true;
}

static inline bool update() {
    bool success = false;
    esp_err_t err;

    NLOGI("writing firmware to %s", upd_part->label);

    int64_t start_time = esp_timer_get_time();

#if CONF_TCP_OTA_VERIFY_MD5
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    /* По документации mbedtls эта функция должна вызываться, но с ней линкер выдает
     * undefined reference. Без нее все работает. */
    /* mbedtls_md5_starts(&md5_ctx); */
#endif // CONF_TCP_OTA_VERIFY_MD5

    uint8_t *buf = malloc(FIRMWARE_BUF_SIZE);
    if (buf == NULL) {
        NLOGE("malloc failed");
        return false;
    }

    size_t loaded = 0;
    while (loaded < header.size) {
        int n = read(csfd, buf, FIRMWARE_BUF_SIZE);
        if (n < 0) {
            NLOGE("read() failed: %s", strerror(errno));
            goto error;
        }

        if (n == 0) {
            NLOGE("expected %zu bytes of firmware, got %zu", header.size, loaded);
            goto error;
        }

#if CONF_TCP_OTA_VERIFY_MD5
        mbedtls_md5_update(&md5_ctx, buf, n);
#endif // CONF_TCP_OTA_VERIFY_MD5

        part_clean = false;

        err = esp_partition_write(upd_part, loaded, buf, n);
        if (err != ESP_OK) {
            NLOGE("esp_partition_write() %s", esp_err_to_name(err));
            goto error;
        }

        loaded += n;
    }

#if CONF_TCP_OTA_VERIFY_MD5
    uint8_t computed_md5[MD5_SIZE];
    mbedtls_md5_finish(&md5_ctx, computed_md5);
#endif // CONF_TCP_OTA_VERIFY_MD5

    float secs = (float)(esp_timer_get_time() - start_time) / 1e6f, avg_speed;
    if (secs == 0)
        /* невозможно */
        avg_speed = INFINITY;
    else
        avg_speed = (float)header.size / secs;

    NLOGI("%zu bytes in %f secs, avg speed %f b/s", header.size, secs, avg_speed);

#if CONF_TCP_OTA_VERIFY_MD5
    if (memcmp(header.md5, computed_md5, MD5_SIZE) != 0) {
        char computed_md5str[MD5STR_SIZE];
        md5str(computed_md5, computed_md5str);
        NLOGE("md5 differs %s (given) != %s (computed)", header.md5str, computed_md5str);
        goto error;
    }
#endif // CONF_TCP_OTA_VERIFY_MD5

    err = esp_ota_set_boot_partition(upd_part);
    if (err != ESP_OK) {
        NLOGE("esp_ota_set_boot_partition() failed: %s", esp_err_to_name(err));
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
            NLOGE("loaded firmware is corrupted, try send file again");

        goto error;
    }

    success = true;

error:
#if CONF_TCP_OTA_VERIFY_MD5
    mbedtls_md5_free(&md5_ctx);
#endif // CONF_TCP_OTA_VERIFY_MD5
    free(buf);
    return success;
}

#if CONF_TCP_OTA_VERIFY_MD5
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
#endif // CONF_TCP_OTA_VERIFY_MD5
