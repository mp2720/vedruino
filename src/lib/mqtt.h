#pragma once

#include "../conf.h"
#include "macro.h"
#include <esp_idf_version.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <mqtt_client.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

PK_EXTERNC_BEGIN

#ifndef TASK_DEFAULT_PRIORITY
#define TASK_DEFAULT_PRIORITY 3
#endif

// флаги собыйтий mqtt
extern EventGroupHandle_t pk_mqtt_event_group;
#define PK_MQTT_CONNECTED_BIT (1 << 0)
#define PK_MQTT_DISCONNECTED_BIT (1 << 1)
#define PK_MQTT_PUBLISHED_BIT (1 << 2)
#define PK_MQTT_SUBSCRIBED_BIT (1 << 3)
#define PK_MQTT_UNSUBSCRIBED_BIT (1 << 4)
#define PK_MQTT_DATA_BIT (1 << 5)
#define PK_MQTT_FAIL_BIT (1 << 6)

// handle для использования других инструментов
extern esp_mqtt_client_handle_t pk_mqtt_client;

// Тип функции вызываемой при получении сообщения на подписанный топик
//(char * topic, char * data, int data_size)
typedef void (*pkCallback_t)(char *, char *, int);

// Структура топика
typedef struct {
    const char *name;      // имя топика
    pkCallback_t callback; // функция которая будет вызвана при получении сообщения на данный топик
    int qos;               // quality of service
} pkTopic_t;

// Подключиться к брокеру
bool pk_mqtt_connect();

// Отписывается от всех и подписывается на len топиков из массива topics. Сортирует переданные
// топики
bool pk_mqtt_set_subscribed_topics(pkTopic_t topics[], int len);

// Отписаться от топика
bool pk_mqtt_unsubscribe_topic(const char *name);

// Отправить сообщение, если data_size равно 0, то считает длинну строки
bool pk_mqtt_publish(const char *topic, const char *data, size_t data_size, int qos, bool retain);

// Выключить mqtt и освободить ресурсы
bool pk_mqtt_delete();

// Приостановить работу mqtt
bool pk_mqtt_stop();

// Возобновить работу mqtt
bool pk_mqtt_resume();

PK_EXTERNC_END
