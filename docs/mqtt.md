# lib/mqtt.h

Библиотека для удобной работы с mqtt. Подписывается на данный массив топиков и вызывает функции при получении сообщений

## Подключиться
Подключение к брокеру, информация о брокере берётся из конфига вида:
```ini
[lib.mqtt]
enabled=(bool)
host=(str)
port=(int)
user=(str)
password=(str)
retry=(int)
```
* `host` - имя хоста
* `port` - порт хоста
* `user` - имя пользователя mqtt
* `password` - пароль пользователя mqtt
* `retry` - кол-во попыток подключиться

Возвращает __1__ при успехе и __0__ при неудаче

```C
bool pk_mqtt_connect();
```

## Callback
Функция которая вызывается при получении сообщения на указанный топик.
После `topic` и `data` всегда идёт нулевой байт, так что можно работать 
как со строками, после завершения функции, они освобождаются. Все callback 
выполняются по очереди, поэтому функция не должна блокироваться на 
продолжительное время.
```C
typedef void (*pkCallback_t)(char *, char *, int);

//Пример
void echo_cb(char *topic, char *data, int data_size) {
    PKLOGD("%s: %s", topic, data);
}
```


## Подписаться

Получает массив топиков вида
```C
typedef struct {
    const char * name;    //имя топика
    pkCallback_t callback;  //функция которая будет вызвана при получении сообщения на данный топик    
    int qos;              //quality of service, для важных вещей ставить 2
} pkTopic_t;

```
и размер данного массива. Отписывается от старых и подписывается на новые.

Переданный массив сортируется. Имена топиков не копируются, и должны быть доступны
всегда

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
#define PK_MQTT_CONNECTED_BIT (1 << 0)
#define PK_MQTT_DISCONNECTED_BIT (1 << 1)
#define PK_MQTT_PUBLISHED_BIT (1 << 2)
#define PK_MQTT_SUBSCRIBED_BIT (1 << 3)
#define PK_MQTT_UNSUBSCRIBED_BIT (1 << 4)
#define PK_MQTT_DATA_BIT (1 << 5)
#define PK_MQTT_FAIL_BIT (1 << 6)
```
Отправка с ожиданием:
```C
xEventGroupClearBits(pk_mqtt_event_group, MQTT_PUBLISHED_BIT);
mqtt_publish(topic, data, 0, 2, 0);
xEventGroupWaitBits(pk_mqtt_event_group, MQTT_PUBLISHED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
```

## Пример

```C
void echo_cb1(char * topic, char * data, int len) {
    PKLOGI("echo_cb1() called");
}

void echo_cb2(char * topic, char * data, int len) {
    PKLOGI("echo_cb2() called");
}

pkTopic_t topics[] = { //список топиков, функций обратного вызова, qos 
    {"esp/test/1", echo_cb1, 2},
    {"esp/test/2", echo_cb2, 2},
};

static void setup_app() {
    //подписаться на все топики из списка 
    pk_mqtt_set_subscribed_topics(topics, sizeof(topics)/sizeof(topics[0]));

    //отправить "started" на топик "esp/start"
    pk_mqtt_publish("esp/start", "started", 0, 2, 0);
}
```