# lib/mqtt.h

Библиотека для удобной работы с mqtt. Подписывается на данный массив топиков и вызывает функции при получении сообщений

## Инициализация
Инициализирует задачу и очередь. Вызывать в первую очередь, единожды.
```C
void mqtt_init();
```

## Подключиться
Подключение к брокеру
* `broker_host` - имя хоста
* `broker_port` - порт хоста
* `username` - имя пользователя mqtt
* `password` - пароль пользователя mqtt

Возвращает __0__ при успехе и __1__ при неудаче

```C
int mqtt_connect(const char * broker_host, uint16_t broker_port, const char * username, const char * password);
```
## Подписаться

Получает массив топиков вида
```C
typedef void (*callback_t)(const char *, const char *, size_t); //(char * topic, char * data, size_t data_size)

typedef struct {
    const char * name;    //имя топика
    callback_t callback;  //функция которая будет вызвана при получении сообщения на данный топик    
    int qos;              //quality of service
} fl_topic_t;

```
и размер данного массива. Отписывается от старых и подписывается на новые.

Переданный массив сортируется.

Возвращает __0__ при успехе и __1__ при неудаче

```C
int mqtt_subscribe_topics(fl_topic_t topics[], int len); 
```

## Отписаться
Отписывается от топика с данным именем

Возвращает __0__ при успехе и __1__ при неудаче
```C
int mqtt_unsubscribe_topic(const char * name);
```

## Отправить
Функция отправки сообщения
* `topic` - имя топика
* `data` - указатель на данные
* `data_size` - размер данных, если строка, то можно указать 0, и размер будет вычислен strlen()
* `qos` - quality of service
* `retain` - флаг брокеру, отправить это сообщение новым устройствам при их подключении

Возвращает __0__ при успехе и __1__ при неудаче
```C
int mqtt_publish(const char * topic, const char * data, size_t data_size, int qos, bool retain); 
```
## Отключиться
Отписывается от текущего брокера
```C
int mqtt_disconnect(); 
```

## Остановить/возобновить
Останавливает и запускает вновь работу mqtt
```C
int mqtt_stop(); 
int mqtt_resume(); 
```
## Проверка подключения
Возвращает `true` если брокер подключён, иначе `false`. Можно использовать для ожидания подключения. 
```C
bool mqtt_is_connected(); 
```