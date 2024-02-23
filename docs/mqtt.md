# lib/mqtt.h

Библиотека для удобной работы с mqtt. Подписывается на данный массив топиков и вызывает функции при получении сообщений

## Подключиться
Подключение к брокеру, информация о брокере берётся из конфига вида:
```
[mqtt]
enabled=(bool)
host=(str)
port=(int)
user=(str)
password=(str)
```
* `host` - имя хоста
* `port` - порт хоста
* `user` - имя пользователя mqtt
* `password` - пароль пользователя mqtt

Возвращает __1__ при успехе и __0__ при неудаче

```C
bool pk_mqtt_connect();
```
## Подписаться

Получает массив топиков вида
```C
//void callback(char * topic, char * data, int data_size) {}
typedef void (*pkCallback_t)(char *, char *, int); 

typedef struct {
    const char * name;    //имя топика
    callback_t callback;  //функция которая будет вызвана при получении сообщения на данный топик    
    int qos;              //quality of service
} pkTopic_t;

```
и размер данного массива. Отписывается от старых и подписывается на новые.

Переданный массив сортируется.

Возвращает __1__ при успехе и __0__ при неудаче

```C
bool pk_mqtt_set_subscribed_topics(pkTopic_t topics[], int len); 
```

## Отписаться
Отписывается от топика с данным именем

Возвращает __1__ при успехе и __0__ при неудаче
```C
bool pk_mqtt_unsubscribe_topic(const char * name);
```

## Отправить
Функция отправки сообщения
* `topic` - имя топика
* `data` - указатель на данные
* `data_size` - размер данных, если строка, то можно указать 0, и размер будет вычислен strlen()
* `qos` - quality of service
* `retain` - флаг брокеру, отправить это сообщение новым устройствам при их подключении

Возвращает __1__ при успехе и __0__ при неудаче
```C
bool pk_mqtt_publish(const char * topic, const char * data, size_t data_size, int qos, bool retain); 
```
## Отключиться
Отписывается от текущего брокера и освобождает ресурсы
```C
bool pk_mqtt_disconnect(); 
```

## Остановить/возобновить
Останавливает и запускает вновь работу mqtt
```C
bool pk_mqtt_stop(); 
bool pk_mqtt_resume(); 
```
## События
Для ожидания подтверждения некоторых событий MQTT используйте EventGroup из freertos
```C
// флаги собыйтий mqtt
extern EventGroupHandle_t pk_mqtt_event_group;
#define MQTT_CONNECTED_BIT (1 << 0)
#define MQTT_DISCONNECTED_BIT (1 << 1)
#define MQTT_PUBLISHED_BIT (1 << 2)
#define MQTT_SUBSCRIBED_BIT (1 << 3)
#define MQTT_UNSUBSCRIBED_BIT (1 << 4)
#define MQTT_DATA_BIT (1 << 5)

```
Отправка с ожиданием:
```C
xEventGroupClearBits(pk_mqtt_event_group, MQTT_PUBLISHED_BIT)
mqtt_publish(topic, data, 0, 2, 0);
xEventGroupWaitBits(pk_mqtt_event_group, MQTT_PUBLISHED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
```

