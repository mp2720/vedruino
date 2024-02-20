#include "mqtt.h"

#include "inc.h"
#include <esp_event.h>
#include <esp_system.h>
#include <esp_wifi.h>
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

static const char *TAG = "mqtt";

esp_mqtt_client_handle_t mqtt_client = NULL;

EventGroupHandle_t pk_mqtt_event_group = NULL;

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(3, 0, 0)
#error ESP_IDF_VERSION < 3.0.0 убери это говно
#endif

static struct {
    pk_topic_t *pairs;
    int size;
} subs_topics = {.pairs = NULL, .size = 0};

static SemaphoreHandle_t topics_mutex;

static int topic_pair_cmp(const void *a, const void *b) {
    return strcmp(((const pk_topic_t *)a)->name, ((const pk_topic_t *)b)->name);
}

bool mqtt_set_subscribed_topics(pk_topic_t topics[], int len) {
    int res = 1;
    if (xSemaphoreTake(topics_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (int i = 0; i < subs_topics.size; i++) { // отписка от предыдущих
            if (esp_mqtt_client_unsubscribe(mqtt_client, subs_topics.pairs[i].name) < 0) {
                PKLOGE("esp_mqtt_client_unsubscribe() error");
                res = 0;
            }
        }
        subs_topics.pairs = topics;
        subs_topics.size = len;
        qsort(subs_topics.pairs, subs_topics.size, sizeof(pk_topic_t), topic_pair_cmp);

        for (int i = 0; i < subs_topics.size; i++) {
            if (esp_mqtt_client_subscribe(mqtt_client, (char *)subs_topics.pairs[i].name,
                                          subs_topics.pairs[i].qos) < 0) {
                PKLOGE("esp_mqtt_client_subscribe() error");
                res = 0;
            }
        }
        xSemaphoreGive(topics_mutex);
    } else {
        PKLOGE("failed to take topics_mutex");
        res = 0;
    }
    return res;
}

static pk_topic_t *find_callback(const char *name) {
    if (xSemaphoreTake(topics_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        pk_topic_t search_tmp_topic = {.name = name};
        pk_topic_t *pair =
            (pk_topic_t *)bsearch(&search_tmp_topic, subs_topics.pairs, subs_topics.size,
                                  sizeof(pk_topic_t), topic_pair_cmp);
        xSemaphoreGive(topics_mutex);
        return pair;
    } else {
        PKLOGE("failed to take topics_mutex");
    }
    return NULL;
}

bool mqtt_unsubscribe_topic(const char *name) {
    if (esp_mqtt_client_unsubscribe(mqtt_client, name) == -1) {
        PKLOGE("esp_mqtt_client_unsubscribe() error");
        return 0;
    }
    return 1;
}

bool mqtt_publish(const char *topic, const char *data, size_t data_size, int qos, bool retain) {
    if (!data_size)
        data_size = strlen(data);

    int res = esp_mqtt_client_publish(mqtt_client, topic, data, data_size, qos, (int)retain);
    if (res < 0) {
        PKLOGE("esp_mqtt_client_publish() error");
        return 0;
    }
    return 1;
}

struct mqtt_callback_item {
    callback_t callback;
    char *topic;
    char *data;
    int data_size;
};

#define CB_TASK_STACK_SIZE 8192
#define CB_TASK_QUEUE_LENGTH 10
static struct {
    struct {
        TaskHandle_t handle;
    } task;
    struct {
        QueueHandle_t handle;
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
        PKLOGI("MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(pk_mqtt_event_group, MQTT_CONNECTED_BIT);
    } break;

    case MQTT_EVENT_DISCONNECTED: {
        PKLOGW("MQTT_EVENT_DISCONNECTED");
        xEventGroupSetBits(pk_mqtt_event_group, MQTT_DISCONNECTED_BIT);
    } break;

    case MQTT_EVENT_SUBSCRIBED: {
        PKLOGV("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        xEventGroupSetBits(pk_mqtt_event_group, MQTT_SUBSCRIBED_BIT);
    } break;

    case MQTT_EVENT_UNSUBSCRIBED: {
        PKLOGV("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        xEventGroupSetBits(pk_mqtt_event_group, MQTT_UNSUBSCRIBED_BIT);
    } break;

    case MQTT_EVENT_PUBLISHED: {
        PKLOGV("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        xEventGroupSetBits(pk_mqtt_event_group, MQTT_PUBLISHED_BIT);
    } break;

    case MQTT_EVENT_DATA: {

        PKLOGV("MQTT_EVENT_DATA: topic: \"%.*s\"", event->topic_len, event->topic);
        xEventGroupSetBits(pk_mqtt_event_group, MQTT_DATA_BIT);

        char *topic_copy = (char *)malloc(event->topic_len + 1);
        if (!topic_copy) {
            PKLOGE("topic_copy malloc() error, no memory, topic size: %d", event->topic_len);
            return;
        }

        memcpy(topic_copy, event->topic, event->topic_len);
        topic_copy[event->topic_len] = '\0';

        pk_topic_t *pair = find_callback(topic_copy);
        if (!pair) {
            PKLOGW("Callback for \"%s\" topic not found", topic_copy);
            free(topic_copy);
            break;
        }

        if (!pair->callback) {
            free(topic_copy);
            break;
        }

        char *data_copy = (char *)malloc(event->data_len + 1);
        if (!data_copy) {
            PKLOGE("data_copy malloc() error, no memory, data size: %d", event->data_len);
            return;
        }

        memcpy(data_copy, event->data, event->data_len);
        data_copy[event->data_len] = '\0';

        struct mqtt_callback_item args = {.callback = pair->callback,
                                          .topic = topic_copy,
                                          .data = data_copy,
                                          .data_size = event->data_len};

        BaseType_t res = xQueueSend(cb_task.queue.handle, &args, pdMS_TO_TICKS(1000));
        if (res != pdTRUE) {
            PKLOGE("Callback queue owerflow");
            free(args.data);
            free(args.topic);
            break;
        }

    } break;

    case MQTT_EVENT_ERROR: {
        PKLOGE("MQTT_EVENT_ERROR");
    } break;

    default: {
        PKLOGV("Other event id:%d", event->event_id);
    } break;
    }
}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
static esp_err_t mqtt_event_handler_legacy(esp_mqtt_event_handle_t event) {
    mqtt_event_handler(NULL, "", 0, (void *)event);
    return ESP_OK;
}
#endif

bool mqtt_connect() {
    cb_task.queue.handle = xQueueCreate(CB_TASK_QUEUE_LENGTH, sizeof(struct mqtt_callback_item));
    if (!cb_task.queue.handle) {
        PKLOGE("MQTT queue create error");
        return 0;
    }
    BaseType_t err = xTaskCreate(mqtt_callback_task, "pk_mqtt_callback_task", CB_TASK_STACK_SIZE,
                                 NULL, TASK_DEFAULT_PRIORITY, &cb_task.task.handle);
    if (err != pdPASS) {
        PKLOGE("MQTT callback task create error: %d", (int)err);
        return 0;
    }

    topics_mutex = xSemaphoreCreateMutex();
    pk_mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));

    mqtt_cfg.host = CONF_MQTT_HOST;
    mqtt_cfg.port = CONF_MQTT_PORT;
    mqtt_cfg.username = CONF_MQTT_USER;
    mqtt_cfg.password = CONF_MQTT_PASSWORD;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
    mqtt_cfg.event_handle = mqtt_event_handler_legacy;
#endif

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        PKLOGE("esp_mqtt_client_init() error");
        return 0;
    }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    esp_err_t res1 = esp_mqtt_client_register_event(
        mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (res1 != ESP_OK) {
        PKLOGE("esp_mqtt_client_register_event() error: %d - %s", (int)res1, esp_err_to_name(res1));
        return 0;
    }
#endif

    esp_err_t res = esp_mqtt_client_start(mqtt_client);
    if (res != ESP_OK) {
        PKLOGE("esp_mqtt_client_start() error: %d - %s", (int)res, esp_err_to_name(res));
        return 0;
    }
    PKLOGI("Connecting to %s:%d", CONF_MQTT_HOST, CONF_MQTT_PORT);

    EventBits_t bits = xEventGroupWaitBits(pk_mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE,
                                           pdFALSE, portMAX_DELAY);
    if (bits & MQTT_CONNECTED_BIT) {
        PKLOGI("Connected to MQTT");
    } else {
        PKLOGE("UNEXPECTED EVENT");
    }

    return 1;
}

bool mqtt_delete() {
    esp_err_t res = esp_mqtt_client_disconnect(mqtt_client);
    if (res != ESP_OK) {
        PKLOGE("esp_mqtt_client_disconnect error: %d - %s", (int)res, esp_err_to_name(res));
    }
    res = esp_mqtt_client_destroy(mqtt_client);
    if (res != ESP_OK) {
        PKLOGE("esp_mqtt_client_destroy error: %d - %s", (int)res, esp_err_to_name(res));
    }
    vTaskDelete(cb_task.task.handle);
    vQueueDelete(cb_task.queue.handle);
    vEventGroupDelete(pk_mqtt_event_group);
    vSemaphoreDelete(topics_mutex);

    mqtt_client = NULL;
    cb_task.task.handle = NULL;
    cb_task.queue.handle = NULL;
    pk_mqtt_event_group = NULL;
    topics_mutex = NULL;
    return 1;
}

bool mqtt_stop() {
    if (!mqtt_client) {
        PKLOGW("Nothing to stop");
        return 0;
    }
    esp_err_t res = esp_mqtt_client_stop(mqtt_client);
    if (res != ESP_OK) {
        PKLOGE("esp_mqtt_client_stop fail: %d - %s", res, esp_err_to_name(res));
        return 0;
    }
    return 1;
}

bool mqtt_resume() {
    if (!mqtt_client) {
        PKLOGW("Nothing to resume");
        return 0;
    }
    esp_err_t res = esp_mqtt_client_start(mqtt_client);
    if (res != ESP_OK) {
        PKLOGE("esp_mqtt_client_start fail: %d - %s", res, esp_err_to_name(res));
        return 0;
    }
    return 1;
}