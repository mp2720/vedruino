#include "ota.h"

#include "../conf.h"
#include "../ota_ver.h"
#include "log.h"
#include "mem.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <cstring>
#include <ctime>

#define OTA_LOGE(...) VDR_LOGE("ota", __VA_ARGS__)
#define OTA_LOGW(...) VDR_LOGW("ota", __VA_ARGS__)
#define OTA_LOGI(...) VDR_LOGI("ota", __VA_ARGS__)
#define OTA_LOGV(...) VDR_LOGV("ota", __VA_ARGS__)
#define OTA_LOGD(...) VDR_LOGD("ota", __VA_ARGS__)

void ota::dump_info() {
    std::time_t ts = OTA_VER_CREATED_AT + CONF_UTC_OFFSET * 60 * 60;
    std::tm *t = std::localtime(&ts);

    OTA_LOGI("OTA firmware loaded:\n"
             "\tid: %d\n"
             "\tuuid: %s\n"
             "\trepo: %s\n"
             "\tcommit id: %s\n"
             "\tboards: %s\n"
             "\tcreated at: %s" // asctime добавит \n
             "\tcreated by: %s\n"
             "\tdescription: %s",
             OTA_VER_ID, OTA_VER_UUID, OTA_VER_REPO_NAME, OTA_VER_COMMIT_ID, OTA_VER_BOARDS,
             std::asctime(t), OTA_VER_CREATED_BY, OTA_VER_DESCRIPTION);
}

static inline void log_http_fail(const HTTPClient &http, const char *url, int code) {
    OTA_LOGE("GET %s failed with code %d: %s", url, code, http.errorToString(code).c_str());
}

static inline void log_json_fail(const DeserializationError &err) {
    OTA_LOGE("ArduinoJson deserialization error: %s", err.c_str());
}

// Как-нибудь без этого можно обойтись, а то уж слишком медленно.
/* static inline bool check_token() { */
/*     OTA_LOGV("checking provided token..."); */

/*     HTTPClient http; */
/*     const char *url = CONF_OTA_HOST "/api/v1/users/me"; */
/*     http.begin(url, CONF_OTA_CERT); */
/*     http.addHeader("X-Token", CONF_OTA_TOKEN); */
/*     int code = http.GET(); */
/*     if (code != 200) { */
/*         log_http_fail(http, url, code); */
/*         return false; */
/*     } */

/*     JsonDocument json; */
/*     auto err = deserializeJson(json, http.getString()); */
/*     http.end(); */
/*     if (err) { */
/*         log_json_fail(err); */
/*         return false; */
/*     } */

/*     const char *username = json["name"].as<const char *>(); */
/*     bool is_board = json["is_board"]; */

/*     const char *type = is_board ? "board" : "dev"; */
/*     OTA_LOGI("token info:\n\tuser: %s\n\ttype: %s\n", username, type); */

/*     if (!is_board) { */
/*         OTA_LOGE("token is signed for dev, but not for board"); */
/*         return false; */
/*     } */

/*     return true; */
/* } */

enum LatestUpdateStatus { LATEST_ERROR, LATEST_NO_UPDATE, LATEST_UPDATE_FOUND };

struct UpdateInfo {
    const char *file_url;
    size_t size;
    char md5_hex[32];
};

static inline LatestUpdateStatus get_latest(UpdateInfo &info) {
    OTA_LOGV("checking latest firmware version...");

    HTTPClient http;
    const char *url = CONF_OTA_HOST "/api/v1/firmwares/latest?repo=" CONF_OTA_REPO;
    http.begin(url, CONF_OTA_CERT);
    http.addHeader("X-Token", CONF_OTA_TOKEN);
    int code = http.GET();
    if (code != 200) {
        log_http_fail(http, url, code);
        return LATEST_ERROR;
    }

    JsonDocument json;
    auto err = deserializeJson(json, http.getString());
    http.end();
    if (err) {
        log_json_fail(err);
        return LATEST_ERROR;
    }

    const char *uuid = json["info"]["uuid"];
    if (std::strcmp(uuid, OTA_VER_UUID) == 0) {
        return LATEST_NO_UPDATE;
    }

    const char *json_file_url = json["bin_url"];
    size_t file_url_size = std::strlen(json_file_url) + 1;
    info.file_url = (char *)mem::malloc(file_url_size);
    std::memcpy((void *)info.file_url, (void *)json_file_url, file_url_size);

    info.size = json["info"]["size"];

    const char *md5 = json["info"]["md5"];
    std::memcpy((void *)info.md5_hex, (void *)md5, sizeof info.md5_hex);

    return LATEST_UPDATE_FOUND;
}

static inline bool fetch_and_save_update(UpdateInfo &info) {
    OTA_LOGV("fetching and writing new firmware")

    HTTPClient http;
    http.begin(info.file_url, CONF_OTA_CERT);
    http.addHeader("X-Token", CONF_OTA_TOKEN);
    int code = http.GET();
    if (code != 200) {
        log_http_fail(http, info.file_url, code);
        return false;
    }

    OTA_LOGI("retreiving file %s and starting OTA update", info.file_url);
    if (!Update.begin(info.size)) {
        OTA_LOGE("failed to start update: %s", Update.errorString());
        return false;
    }

    unsigned long start_time = millis();

    uint8_t buf[128]{};
    WiFiClient *stream = http.getStreamPtr();
    int total_size;
    int len = total_size = http.getSize();
    while (http.connected() && (len > 0 || len == -1)) {
        size_t size = stream->available();
        if (size) {
            int block_size = stream->readBytes(buf, std::min(size, sizeof(buf)));
            Update.write(buf, block_size);

            if (len > 0)
                len -= block_size;
        }
    }

    http.end();
    if (!Update.end(true)) {
        OTA_LOGE("failed to finish update");
        return false;
    }

    float duration_s = (millis() - start_time) / 1000.0f;
    float avg_speed;
    if (duration_s == 0)
        // impossible
        avg_speed = 99999999999.0f;
    else
        avg_speed = float(total_size) / duration_s;

    OTA_LOGI("loaded %d bytes in %f secs with avg speed: %f b/s", total_size, duration_s, avg_speed);

    return true;
}

enum UpdateStatus { STATUS_ERROR, STATUS_NO_UPDATES, STATUS_UPDATED };

UpdateStatus try_update_once() {
    OTA_LOGI("starting update check")

    /* if (!check_token()) */
    /*     return STATUS_ERROR; */

    UpdateInfo info{};
    auto status = get_latest(info);

    switch (status) {
    case LATEST_ERROR:
        return STATUS_ERROR;

    case LATEST_NO_UPDATE:
        return STATUS_NO_UPDATES;

    case LATEST_UPDATE_FOUND:
        return fetch_and_save_update(info) ? STATUS_UPDATED : STATUS_ERROR;
    }
}

static inline void blink_error_inf() {
    while (1) {
    }
}

static inline void reboot() {
    ESP.restart();
}

void ota::update() {
    int attempts = CONF_OTA_RETRIES;
    while (attempts--) {
        switch (try_update_once()) {
        case STATUS_ERROR:
            OTA_LOGI("failed, %d attempts left", attempts);
            break;
        case STATUS_NO_UPDATES:
            OTA_LOGI("this is the last version - no updates required");
            return;
        case STATUS_UPDATED:
            OTA_LOGI("update finished successfully, rebooting...")
            reboot();
        }
    }

    blink_error_inf();
}
