#include "mqtt.h"

#include "log.h"
#include "macro.h"
#include <esp_event.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <lwip/dns.h>
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "MQTT";

static bool mqtt_connect_flag = 0;

esp_mqtt_client_handle_t mqtt_client = NULL;

static struct {
    fl_topic_t *pairs;
    int size;
} subs_topics = {.pairs = NULL, .size = 0};

static SemaphoreHandle_t topics_mutex;

static int topic_pair_cmp(const void *a, const void *b) {
    return strcmp(((const fl_topic_t *)a)->name, ((const fl_topic_t *)b)->name);
}

int mqtt_subscribe_topics(fl_topic_t topics[], int len) {
    int res = 0;
    if (xSemaphoreTake(topics_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < subs_topics.size; i++) {
            if (esp_mqtt_client_unsubscribe(mqtt_client, subs_topics.pairs[i].name) == -1) {
                DFLT_LOGE(TAG, "esp_mqtt_client_unsubscribe() error");
                res = 1;
            }
        }
        subs_topics.pairs = topics;
        subs_topics.size = len;
        qsort(subs_topics.pairs, subs_topics.size, sizeof(fl_topic_t), topic_pair_cmp);
        for (int i = 0; i < subs_topics.size; i++) {
            if (esp_mqtt_client_subscribe(mqtt_client, (char *)subs_topics.pairs[i].name,
                                          subs_topics.pairs[i].qos) == -1) {
                DFLT_LOGE(TAG, "esp_mqtt_client_subscribe() error");
                res = 1;
            }
        }
        xSemaphoreGive(topics_mutex);
    } else {
        DFLT_LOGE(TAG, "failed to take topics_mutex");
        res = 1;
    }
    return res;
}

static fl_topic_t *find_callback(const char *name) {
    if (xSemaphoreTake(topics_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        fl_topic_t search_tmp_topic = {.name = name};
        fl_topic_t *pair =
            (fl_topic_t *)bsearch(&search_tmp_topic, subs_topics.pairs, subs_topics.size,
                                  sizeof(fl_topic_t), topic_pair_cmp);
        xSemaphoreGive(topics_mutex);
        return pair;
    } else {
        DFLT_LOGE(TAG, "failed to take topics_mutex");
    }
    return NULL;
}

int mqtt_unsubscribe_topic(const char *name) {
    if (esp_mqtt_client_unsubscribe(mqtt_client, name) == -1) {
        DFLT_LOGE(TAG, "esp_mqtt_client_unsubscribe() error");
        return 1;
    }
    return 0;
}

int mqtt_publish(const char *topic, const char *data, size_t data_size, int qos, bool retain) {
    if (!data_size)
        data_size = strlen(data);

    int res = esp_mqtt_client_publish(mqtt_client, topic, data, data_size, qos, (int)retain);
    if (res < 0) {
        DFLT_LOGE(TAG, "esp_mqtt_client_publish() error");
        return 1;
    }
    return 0;
}

struct mqtt_callback_item {
    callback_t callback;
    char *topic;
    char *data;
    size_t data_size;
};

#define CB_TASK_STACK_SIZE 8192
#define CB_TASK_QUEUE_LENGTH 10
static struct {
    struct {
        TaskHandle_t handle;
        StaticTask_t buffer;
        StackType_t stack[CB_TASK_STACK_SIZE];
    } task;
    struct {
        QueueHandle_t handle;
        StaticQueue_t buffer;
        uint8_t storage[CB_TASK_QUEUE_LENGTH * sizeof(struct mqtt_callback_item)];
    } queue;
} cb_task;

static void mqtt_callback_task(UNUSED void *pvParameters) {
    struct mqtt_callback_item args;
    while (1) {
        if (xQueueReceive(cb_task.queue.handle, &args, portMAX_DELAY)) {
            (args.callback)(args.topic, args.data, args.data_size);
            free(args.data);
            free(args.topic);
        }
    }
    vTaskDelete(NULL);
}

static void mqtt_event_handler(UNUSED void *handler_args, UNUSED esp_event_base_t base,
                               UNUSED int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {

    case MQTT_EVENT_CONNECTED: {
        DFLT_LOGD(TAG, "MQTT_EVENT_CONNECTED");
        vTaskResume(cb_task.task.handle);
        mqtt_connect_flag = 1;
    } break;

    case MQTT_EVENT_DISCONNECTED: {
        DFLT_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
        vTaskSuspend(cb_task.task.handle);
        mqtt_connect_flag = 0;
    } break;

    case MQTT_EVENT_SUBSCRIBED: {
        DFLT_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    } break;

    case MQTT_EVENT_UNSUBSCRIBED: {
        DFLT_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    } break;

    case MQTT_EVENT_PUBLISHED: {
        DFLT_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    } break;

    case MQTT_EVENT_DATA: {

        DFLT_LOGD(TAG, "MQTT_EVENT_DATA");
        DFLT_LOGV(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
        DFLT_LOGV(TAG, "DATA=%.*s\r\n", event->data_len, event->data);

        char *topic_copy = (char *)malloc(event->topic_len + 1);
        if (!topic_copy) {
            DFLT_LOGE(TAG, "topic_copy malloc() error, no memory, topic size: %d",
                      event->topic_len);
            return;
        }

        memcpy(topic_copy, event->topic, event->topic_len);
        topic_copy[event->topic_len] = '\0';

        fl_topic_t *pair = find_callback(topic_copy);
        if (!pair) {
            DFLT_LOGW(TAG, "Callback for \"%s\" topic not found", topic_copy);
            free(topic_copy);
            break;
        }

        if (!pair->callback) {
            free(topic_copy);
            break;
        }

        char *data_copy = (char *)malloc(event->data_len + 1);
        if (!data_copy) {
            DFLT_LOGE(TAG, "data_copy malloc() error, no memory, data size: %d", event->data_len);
            return;
        }

        memcpy(data_copy, event->data, event->data_len);
        data_copy[event->data_len] = '\0';

        struct mqtt_callback_item args = {.callback = pair->callback,
                                          .topic = topic_copy,
                                          .data = data_copy,
                                          .data_size = (size_t)event->data_len};

        BaseType_t res = xQueueSend(cb_task.queue.handle, &args, pdMS_TO_TICKS(1000));
        if (res != pdTRUE) {
            DFLT_LOGE(TAG, "Callback queue owerflow");
            free(args.data);
            free(args.topic);
            break;
        }

    } break;

    case MQTT_EVENT_ERROR: {
        DFLT_LOGE(TAG, "MQTT_EVENT_ERROR");
    } break;

    default: {
        DFLT_LOGD(TAG, "Other event id:%d", event->event_id);
    } break;
    }
}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
static esp_err_t mqtt_event_handler_legacy(esp_mqtt_event_handle_t event) {
    mqtt_event_handler(NULL, "", 0, (void *)event);
    return ESP_OK;
}
#endif

void mqtt_init() {
    cb_task.queue.handle =
        xQueueCreateStatic(CB_TASK_QUEUE_LENGTH, sizeof(struct mqtt_callback_item),
                           cb_task.queue.storage, &cb_task.queue.buffer);

    cb_task.task.handle =
        xTaskCreateStatic(mqtt_callback_task, "fl_mqtt_callback_task", CB_TASK_STACK_SIZE, NULL,
                          TASK_DEFAULT_PRIORITY, cb_task.task.stack, &cb_task.task.buffer);
    vTaskSuspend(cb_task.task.handle);
    topics_mutex = xSemaphoreCreateMutex();

    DFLT_LOGI(TAG, "initialized");
}

int mqtt_connect(const char *broker_host, uint16_t broker_port, const char *username,
                 const char *password) {

    esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    mqtt_cfg.broker.address.hostname = broker_host, mqtt_cfg.broker.address.port = broker_port,
    mqtt_cfg.credentials.username = username,
    mqtt_cfg.credentials.authentication.password = password,
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    mqtt_cfg.host = broker_host, mqtt_cfg.port = broker_port, mqtt_cfg.username = username,
    mqtt_cfg.password = password,
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(3, 0, 0)
    mqtt_cfg.event_handle = mqtt_event_handler_legacy, mqtt_cfg.host = broker_host,
    mqtt_cfg.port = broker_port, mqtt_cfg.username = username, mqtt_cfg.password = password,
#else
#error Old ESP-IDF version
#endif

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        DFLT_LOGE(TAG, "esp_mqtt_client_init() error");
        return 1;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    int res1 = esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                              mqtt_event_handler, NULL);
    if (res1 != ESP_OK) {
        DFLT_LOGE(TAG, "esp_mqtt_client_register_event() error: %d", (int)res1);
        return 1;
    }
#endif

    if (!cb_task.task.handle || !cb_task.queue.handle) {
        DFLT_LOGE(TAG, "Task or queue not initialized, run mqtt_init()");
        return 1;
    }

    esp_err_t res = esp_mqtt_client_start(mqtt_client);
    if (res != ESP_OK) {
        DFLT_LOGE(TAG, "esp_mqtt_client_start() error: %d", (int)res);
        return 1;
    }

    return 0;
}

int mqtt_disconnect() {
    esp_err_t res = esp_mqtt_client_disconnect(mqtt_client);
    if (res != ESP_OK) {
        DFLT_LOGE(TAG, "esp_mqtt_client_disconnect error: %d", (int)res);
        return 1;
    }
    if (cb_task.task.handle)
        vTaskSuspend(cb_task.task.handle);
    return 0;
}

int mqtt_stop() {
    if (!mqtt_client) {
        DFLT_LOGW(TAG, "Nothing to stop");
        return 1;
    }
    esp_err_t res = esp_mqtt_client_stop(mqtt_client);
    if (res != ESP_OK) {
        DFLT_LOGE(TAG, "esp_mqtt_client_stop fail: %d", res);
        return 1;
    }
    if (cb_task.task.handle)
        vTaskSuspend(cb_task.task.handle);
    return 0;
}

int mqtt_resume() {
    if (!mqtt_client) {
        DFLT_LOGW(TAG, "Nothing to resume");
        return 1;
    }
    if (cb_task.task.handle)
        vTaskResume(cb_task.task.handle);
    esp_err_t res = esp_mqtt_client_start(mqtt_client);
    if (res != ESP_OK) {
        DFLT_LOGE(TAG, "esp_mqtt_client_start fail: %d", res);
        return 1;
    }
    return 0;
}

bool mqtt_is_connected() {
    return mqtt_connect_flag;
}
