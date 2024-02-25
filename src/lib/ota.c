/*
 * Тот миша, который сегодня, слишком жирный и будет медленно таскать ноутбуки к платам.
 * Поэтому я его опять автоматизировал.
 */

#include "inc.h"

#if CONF_LIB_OTA_ENABLED

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

#if CONF_LIB_OTA_VERIFY_MD5
#include <mbedtls/md5.h>
#endif // CONF_LIB_OTA_VERIFY_MD5

static const char *TAG = "ota";

#define SERVER_TASK_STACK_SIZE 4096
#define SERVER_TASK_LOW_PRIORITY 1
// Приоритет выше чем у wifi ставить смысла нет.
#define SERVER_TASK_HIGH_PRIORITY 23

#define FIRMWARE_BUF_SIZE 4096
#define FIRMWARE_BLK_SIZE 4096

#define SERVER_PORT 5256

#define MAGIC "2720OTAUPDATE"
#define MAGIC_SIZE (sizeof(MAGIC) - 1)
#define MD5_SIZE 16
#define MD5STR_SIZE (MD5_SIZE * 2)

#define RESP_OK '+'
#define RESP_ERR '-'

// Время на запись ответа в сокет и закрытие.
#define DELAY_BEFORE_REBOOT_MS 100

typedef enum {
    PK_OTA_RESP_OK,
    PK_OTA_RESP_INTERNAL_ERR,
    PK_OTA_RESP_MALFORMED_REQ,
    PK_OTA_RESP_FIRMWARE_UNEXPECTED_END,
    PK_OTA_RESP_FIRMWARE_TOO_BIG,
    PK_OTA_RESP_ESP_VERIFY_ERR,
#if CONF_OTA_VERIFY_MD5
    PK_OTA_RESP_MD5_VERIFY_ERR
#endif // CONF_OTA_VERIFY_MD5
} pkOtaResp_t;

typedef enum { PK_OTA_PART_CLEAN, PK_OTA_PART_NEED_ERASE, PK_OTA_PART_ERR } pkOtaPartStatus_t;

static const esp_partition_t *upd_part;
static size_t part_bytes_written;

static uint8_t server_task_stack[SERVER_TASK_STACK_SIZE];
static StaticTask_t server_task_st;

// запрос должен выглядеть так: 2720OTAUDPATE<size><md5><data>
// md5 и size для data
static struct {
    char magic[MAGIC_SIZE];
    uint8_t md5[MD5_SIZE];
    char md5str[MD5STR_SIZE];
    uint32_t size;
} header;

static void server_task(void *);
static pkOtaResp_t handle_req(pkTcpClient_t chd);
static pkOtaPartStatus_t check_upd_part(void);
// bytes может быть больше размера раздела, тогда он сотрётся полностью
static bool erase_upd_part(size_t bytes);
static void md5str(const uint8_t md5[MD5_SIZE], char out_str[MD5STR_SIZE]);
static char nibble_str(uint8_t n);

bool ota_server_start() {
    TaskHandle_t thd =
        xTaskCreateStatic(&server_task, "ota", SERVER_TASK_STACK_SIZE, NULL,
                          SERVER_TASK_LOW_PRIORITY, server_task_stack, &server_task_st);
    if (thd == NULL) {
        PKLOGE("xTaskCreateStatic failed");
        return false;
    }
    return true;
}

static void server_task(PK_UNUSED void *p) {
    upd_part = esp_ota_get_next_update_partition(NULL);
    if (upd_part == NULL) {
        PKLOGE("no partition found for next update");
        goto fatal_err;
    }

    // Если не проходит, значит размер блока неправильный
    PK_ASSERT(upd_part->size % FIRMWARE_BLK_SIZE == 0);

    PKLOGI("next update partition: %s", upd_part->label);

    pkOtaPartStatus_t part_status = check_upd_part();
    switch (part_status) {
    case PK_OTA_PART_CLEAN:
        break;
    case PK_OTA_PART_NEED_ERASE:
        if (!erase_upd_part(upd_part->size))
            goto fatal_err;
        break;
    case PK_OTA_PART_ERR:
        goto fatal_err;
    }

    pkTcpServer_t shd = pk_tcp_server(SERVER_PORT, 1);
    if (shd == PK_SOCKERR)
        goto fatal_err;

    PKLOGI("listening on port " PK_STRINGIZE(SERVER_PORT));

    while (1) {
        pkIpAddr_t cl_addr;
        char cl_addr_str[PK_IP_ADDR_STR_LEN];
        pkTcpClient_t chd = pk_tcp_accept(shd, &cl_addr);
        if (chd == PK_SOCKERR)
            continue;

        pk_ip_addr2str(cl_addr, cl_addr_str);

        vTaskPrioritySet(NULL, SERVER_TASK_HIGH_PRIORITY);
        /* sysmon_pause(); */

        PKLOGI("request from %s", cl_addr_str);

        char resp_status_byte;
        const char *err_msg;

        pkOtaResp_t resp = handle_req(chd);
        switch (resp) {
        case PK_OTA_RESP_OK:
            resp_status_byte = RESP_OK;
            pk_tcp_sendn(chd, &resp_status_byte, 1);
            pk_sock_close(chd);

            PKLOGW("rebooting now");
            vTaskDelay(pdMS_TO_TICKS(DELAY_BEFORE_REBOOT_MS));

            // noreturn
            esp_restart();

        case PK_OTA_RESP_INTERNAL_ERR:
            err_msg = "internal error";
            break;
        case PK_OTA_RESP_MALFORMED_REQ:
            err_msg = "malformed request";
            break;
        case PK_OTA_RESP_ESP_VERIFY_ERR:
            err_msg = "esp verification error";
            break;
        case PK_OTA_RESP_FIRMWARE_TOO_BIG:
            err_msg = "firmware is too big";
            break;
        case PK_OTA_RESP_FIRMWARE_UNEXPECTED_END:
            err_msg = "unexpected end of firmware in request";
            break;
#if CONF_OTA_VERIFY_MD5
        case PK_OTA_RESP_MD5_VERIFY_ERR:
            err_msg = "md5 verification error";
            break;
#endif // CONF_OTA_VERIFY_MD5
        }

        PKLOGE("error handling request from %s: %s", cl_addr_str, err_msg);

        resp_status_byte = RESP_ERR;
        pk_tcp_sendn(chd, &resp_status_byte, 1);
        size_t err_msg_len = strlen(err_msg);
        pk_tcp_sendn(chd, err_msg, err_msg_len);
        pk_sock_close(chd);

        if (part_bytes_written != 0) {
            if (!erase_upd_part(part_bytes_written)) {
                pk_sock_close(shd);
                goto fatal_err;
            }
            part_bytes_written = 0;
        }

        vTaskPrioritySet(NULL, SERVER_TASK_LOW_PRIORITY);
        /* sysmon_resume(); */
    }

fatal_err:
    PKLOGE("fatal error, shutting down the server");
    vTaskDelete(NULL);
}

static pkOtaResp_t handle_req(pkTcpClient_t chd) {
    if (!pk_tcp_recvn(chd, header.magic, MAGIC_SIZE) ||
        memcmp(header.magic, MAGIC, MAGIC_SIZE) != 0)
        return PK_OTA_RESP_MALFORMED_REQ;

    if (!pk_tcp_recvn(chd, &header.size, 4))
        return PK_OTA_RESP_MALFORMED_REQ;
    header.size = htonl(header.size);

    if (header.size > upd_part->size)
        return PK_OTA_RESP_FIRMWARE_TOO_BIG;

    if (!pk_tcp_recvn(chd, header.md5, MD5_SIZE))
        return PK_OTA_RESP_MALFORMED_REQ;

    md5str(header.md5, header.md5str);
    PKLOGI("new firmare size=%zu, md5=%.*s", header.size, MD5STR_SIZE, header.md5str);

    void *buf = malloc(FIRMWARE_BUF_SIZE);
    if (buf == NULL)
        return PK_OTA_RESP_INTERNAL_ERR;

#if !CONF_OTA_VERIFY_MD5
    PKLOGI("md5 ignored due to ota:verify_md5=false");
#endif // !CONF_OTA_VERIFY_MD5

#if CONF_OTA_VERIFY_MD5
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    /* По документации mbedtls эта функция должна вызываться, но с ней линкер выдает
     * undefined reference. Без нее все работает. */
    /* mbedtls_md5_starts(&md5_ctx); */
#endif // CONF_OTA_VERIFY_MD5

    int64_t write_start_time = esp_timer_get_time();

    esp_err_t err;
    pkOtaResp_t resp;
    part_bytes_written = 0;
    while (part_bytes_written < header.size) {
        ssize_t rcv = pk_tcp_recv(chd, buf, FIRMWARE_BUF_SIZE);
        if (rcv == PK_SOCKERR) {
            resp = PK_OTA_RESP_INTERNAL_ERR;
            goto end;
        }

        if (rcv == 0) {
            resp = PK_OTA_RESP_FIRMWARE_UNEXPECTED_END;
            goto end;
        }

#if CONF_OTA_VERIFY_MD5
        mbedtls_md5_update(&md5_ctx, buf, rcv);
#endif // CONF_OTA_VERIFY_MD5

        err = esp_partition_write_raw(upd_part, part_bytes_written, buf, rcv);
        part_bytes_written += rcv;
        if (err != ESP_OK) {
            PKLOGE("esp_partition_write_raw() %s", esp_err_to_name(err));
            resp = PK_OTA_RESP_INTERNAL_ERR;
            goto end;
        }
    }

    float secs = (float)(esp_timer_get_time() - write_start_time) / 1e6f;
    float avg_speed;
    if (secs == 0)
        /* невозможно */
        avg_speed = INFINITY;
    else
        avg_speed = (float)header.size / secs;

    float size_f, avg_speed_f;
    char size_suf[PK_BISUFFIX_SIZE], avg_speed_suf[PK_BISUFFIX_SIZE];
    pk_hum_size((float)header.size, &size_f, size_suf);
    pk_hum_size(avg_speed, &avg_speed_f, avg_speed_suf);

    PKLOGI("%.1f%sb in %.1f secs, avg speed %.1f%sb/s", size_f, size_suf, secs, avg_speed_f,
           avg_speed_suf);

#if CONF_OTA_VERIFY_MD5
    uint8_t computed_md5[MD5_SIZE];
    mbedtls_md5_finish(&md5_ctx, computed_md5);

    if (memcmp(computed_md5, header.md5, MD5_SIZE) != 0) {
        char computed_md5_str[MD5_SIZE];
        md5str(computed_md5, computed_md5_str);
        PKLOGE("md5 mismatch: %.*s != %.*s", MD5STR_SIZE, computed_md5_str, MD5STR_SIZE,
               header.md5str);
        resp = PK_OTA_RESP_MD5_VERIFY_ERR;
        goto end;
    }
#endif // CONF_OTA_VERIFY_MD5

    err = esp_ota_set_boot_partition(upd_part);
    if (err != ESP_OK) {
        PKLOGE("esp_ota_set_boot_partition() %s", esp_err_to_name(err));
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
            resp = PK_OTA_RESP_ESP_VERIFY_ERR;
        else
            resp = PK_OTA_RESP_INTERNAL_ERR;
        goto end;
    }

    resp = PK_OTA_RESP_OK;

end:
    free(buf);
#if CONF_OTA_VERIFY_MD5
    mbedtls_md5_free(&md5_ctx);
#endif // CONF_OTA_VERIFY_MD5
    return resp;
}

static pkOtaPartStatus_t check_upd_part(void) {
    PKLOGD("checking if %s is clean", upd_part->label);
    uint8_t *buf = malloc(FIRMWARE_BLK_SIZE);
    if (buf == NULL) {
        PKLOGE("malloc(" PK_STRINGIZE(FIRMWARE_BLK_SIZE) ")");
        return PK_OTA_PART_ERR;
    }

    for (size_t offs = 0; offs < upd_part->size; offs += FIRMWARE_BLK_SIZE) {
        esp_err_t err = esp_partition_read(upd_part, offs, buf, FIRMWARE_BLK_SIZE);
        if (err != ESP_OK) {
            PKLOGE("esp_partition_read() %s", esp_err_to_name(err));
            free(buf);
            return PK_OTA_PART_ERR;
        }

        bool clean = true;
        for (int i = 0; i < FIRMWARE_BLK_SIZE; ++i)
            clean = clean && (buf[i] == 0xff);

        if (!clean) {
            PKLOGD("%s is not clean", upd_part->label);
            free(buf);
            return PK_OTA_PART_NEED_ERASE;
        }
    }

    PKLOGD("%s is clean", upd_part->label);
    free(buf);
    return PK_OTA_PART_CLEAN;
}

static bool erase_upd_part(size_t bytes) {
    if (bytes > upd_part->size)
        bytes = upd_part->size;

    size_t blks = bytes / FIRMWARE_BLK_SIZE;
    if (bytes % FIRMWARE_BLK_SIZE != 0)
        ++blks;

    PKLOGD("erasing %zu (%zu blocks) bytes of %s", bytes, blks, upd_part->label);
    esp_err_t err = esp_partition_erase_range(upd_part, 0, blks * FIRMWARE_BLK_SIZE);
    if (err != ESP_OK) {
        PKLOGE("esp_partition_erase_range() %s", esp_err_to_name(err));
        return false;
    }

    PKLOGD("erased");
    return true;
}

static void md5str(const uint8_t md5[MD5_SIZE], char out_str[MD5STR_SIZE]) {
    for (int i = 0; i < MD5_SIZE; ++i) {
        uint8_t low = md5[i] & 0x0F;
        uint8_t high = (md5[i] & 0xF0) >> 4;
        out_str[2 * i] = nibble_str(high);
        out_str[2 * i + 1] = nibble_str(low);
    }
}

static inline char nibble_str(uint8_t n) {
    if (n <= 9)
        return '0' + n;
    else
        return 'a' + n - 10;
}

#endif // CONF_LIB_OTA_ENABLED
