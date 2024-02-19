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

Возвращает __1__ при успехе и __0__ при неудаче

```C
bool mqtt_connect(const char * broker_host, uint16_t broker_port, const char * username, const char * password);
```
## Подписаться

Получает массив топиков вида
```C
//void callback(char * topic, char * data, int data_size) {}
typedef void (*callback_t)(char *, char *, int); 

typedef struct {
    const char * name;    //имя топика
    callback_t callback;  //функция которая будет вызвана при получении сообщения на данный топик    
    int qos;              //quality of service
} fl_topic_t;

```
и размер данного массива. Отписывается от старых и подписывается на новые.

Переданный массив сортируется.

Возвращает __1__ при успехе и __0__ при неудаче

```C
bool mqtt_subscribe_topics(fl_topic_t topics[], int len); 
```

## Отписаться
Отписывается от топика с данным именем

Возвращает __1__ при успехе и __0__ при неудаче
```C
bool mqtt_unsubscribe_topic(const char * name);
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
bool mqtt_publish(const char * topic, const char * data, size_t data_size, int qos, bool retain); 
```
## Отключиться
Отписывается от текущего брокера
```C
bool mqtt_disconnect(); 
```

## Остановить/возобновить
Останавливает и запускает вновь работу mqtt
```C
bool mqtt_stop(); 
bool mqtt_resume(); 
```
## Проверка подключения
Возвращает __1__ если брокер подключён, иначе __0__. Можно использовать для ожидания подключения. 
```C
bool mqtt_is_connected(); 
```