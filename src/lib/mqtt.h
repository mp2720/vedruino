#ifndef FL_ESP_MQTT_H
#define FL_ESP_MQTT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_idf_version.h"
#include "mqtt_client.h"

#ifndef TASK_DEFAULT_PRIORITY
    #define TASK_DEFAULT_PRIORITY 3
#endif

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(3, 0, 0)
    #error ESP_IDF_VERSION < 3.0.0 убери это говно
#endif

//handle для использования других инструментов
extern esp_mqtt_client_handle_t mqtt_client; 

//Тип функции вызываемой при получении сообщения на подписанный топик
typedef void (*callback_t)(const char *, const char *, size_t); //(char * topic, char * data, size_t data_size)

//Структура топика
typedef struct {
    const char * name;    //имя топика
    callback_t callback;  //функция которая будет вызвана при получении сообщения на данный топик    
    int qos;              //quality of service
} fl_topic_t;

//Инициализирует инструменты. Запускать единожды
void mqtt_init();

//Подключиться к брокеру на mqtt://broker_host:broker_port, с аутентификацией
int mqtt_connect(const char * broker_host, uint16_t broker_port, const char * username, const char * password);

//Отписывается от всех и подписывается на len топиков из массива topics. Сортирует переданные топики
void mqtt_subscribe_topics(fl_topic_t topics[], int len); 

//Отписаться от топика
void mqtt_unsubscribe_topic(const char * name);

//Отправить сообщение, если data_size равно 0, то считает длинну строки
void mqtt_publish(const char * topic, const char * data, size_t data_size, int qos, int retain); 

//Отключиться от брокера
int mqtt_disconnect(); 

//Приостановить работу mqtt
void mqtt_stop(); 

//Возобновить работу mqtt
void mqtt_resume(); 

//Возвращает 1 если брокер подключён, иначе 0
bool mqtt_is_connected(); 

#endif

/*

работает на arduino-esp32 от 1.6.0 и новее

*/