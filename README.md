# Подготовка
Нужны `make`, `pyserial`, `arduino-cli` и пакеты Arduino для ESP32.

## Доступ к serial usb
Создание группы `usb_serial` и добавление пользователя:
```bash
groupadd usb_serial
usermod -a -G usb_serial $USER
```

Правило для udev (файл `/etc/udev/rules.d/83-esp32.rules`), которое даёт доступ группе `usb_serial`
на чтение/запись устройства:
```
KERNEL=="ttyUSB[0-9]*", SUBSYSTEM=="tty", SUBSYSTEMS=="usb-serial", GROUP="usb_serial", MODE="0660"
```

Внимание! Это правило применятся ко всем последовательным USB устройствам.
Чтобы ограничить его, можно использовать ATTRS.

## Конфигурация
Нужно скопировать `config.ini.example` в `config.ini` и указать нужные значения.

FQBN посмотреть так:
```bash
arduino-cli board listall BOARD_NAME
```

Затем:
```bash
make setup
```

# clangd
В начале использования и после добавления любого C/C++ файла, который должен обрабатываться clangd
нужно сгенерировать новый `compile_commands.json`:
```bash
make updcc
```

Запускать `clangd` так:
```bash
clangd -query-driver=%ARDUINO%/packages/**"
```

Где `%ARDUINO%` - путь к папке с файлами ведруины, например `/home/user/.arduino15`.

Для `nvim.lsp-config`:
```lua
require 'lspconfig'.clangd.setup({
    cmd = {
        "clangd",
        "--header-insertion=never", -- Без этого вставляет странные инклюды при использовании автокомплита.
        "-query-driver=%ARDUINO%/packages/**"
    },
})
```

## In included files ... typedef redefinition
Если такую ошибку будет выдавать clangd при подключении заголовочных файлов (`WiFi.h`), то можно
выключить сообщения о переопределении typedef, написав в `.clangd`:
```yaml
Diagnostics:
  Suppress: 'err_redefinition_different_typedef'
```

Список ошибок и предупреждений можно найти в этом [файле](https://github.com/llvm/llvm-project/blob/main/clang/include/clang/Basic/DiagnosticSemaKinds.td)
или где-то рядом с ним.

Но сейчас конкретно эта проблема решена другим способом: передавать clangd `-DSSIZE_MAX=...`
(в `compile_commands.py`).

# [arduino-language-server](https://github.com/arduino/arduino-language-server)
Нужно сгенерировать конфиг, если его еще нет:
```bash
arduino-cli config init
```

Дальше всё понятно.

# TCP OTA
На плате работает TCP сервер, принимающий запросы на обновление.
В `config.ini` его можно выключить (`tcp_ota.enabled=false`) и задать порт
(по умолчанию `tcp_ota.port=5256`).

С компьютера отправить запрос на обновление можно так (берёт данные из конфига):
```
make ota
```

Работает довольно быстро (у меня ~700kB за 7 секунд загружает, по usb ~6.4 секунды).
Можно ускорить если использовать сжатие и многопоточность, но это пока не реализовано.



# lib/log.h

Набор макросов для работы логов вида `ESP_LOGX(TAG, format, ...)`, работает как `printf` но оформляет цвет, заголовок сообщения и переносит строку.

```
E [<TAG>] <message>
W [<TAG>] <message>
I [<TAG>] <message>
D [<TAG>] <message>
V [<TAG>] <message>
```
Где вместо `<TAG>` передаётся название модуля, а `<message>` сообщение.

TAG определяется в начале файла следующим образом:
```C
static const char * TAG = "MY_MODULE";
```

Ошибка:
```C
ESP_LOGE(const char * TAG, const char * format, ...)
```

Предупреждение:
```C
ESP_LOGW(const char * TAG, const char * format, ...)
```

Информатиция:
```C
ESP_LOGI(const char * TAG, const char * format, ...)
```

Отладка:
```C
ESP_LOGD(const char * TAG, const char * format, ...)
```

Дополнительные :
```C
ESP_LOGV(const char * TAG, const char * format, ...)
```

## Уровень логирования 
Уровень логгирования определяется в config.ini
* __0__ - нет логов
* __1__ - error
* __2__ - warning
* __3__ - info
* __4__ - debug
* __5__ - verbose

Чем больше уровень, тем менее значимые сообщения будут приходить
```C
#ifndef CONF_LOG_LEVEL
    #define LIBLOG_LEVEL 5
#else
    #define LIBLOG_LEVEL CONF_LOG_LEVEL
#endif
```

## log_output
Указатель на функцию с printf-подобным интерфейсом, по умолчанию ему и равен.
При изменении на `tcp_log_printf` логи будут передаваться новой функции
```C
typedef int (*printf_like_t) (const char *, ...);

extern printf_like_t log_output; 
```

# lib/tcp.h
Набор функций для работы с сокетами

## Подключиться 
Функция получает имя сервера и порт, пытается подключиться, и при успехе возвращает дескриптор сокета, который используется в дальнейшем, иначе возвращает -1.

Именем хоста может быть IP адрес или домен

Портом может быть число или название сервиса (см. /etc/services)
```C
int tcp_connect(const char * host_name, const char * host_port);
```
## Закрыть
Получает сокет, и отключает его от сервера. Обязательно использовать перед перезаписью дескриптора сокета
```C
int tcp_close(int socket);
```

## Отправить
Отправляет сообщение `payload` на данный сокет. В `len` нужно указать длинну сообщения в байтах, если это строка, то можно оставить `len` равным 0, тогда длина будет вычислена с помощью `strlen()`
```C
int tcp_send(int socket, const char * payload, size_t len);
```

## tcp_printf()
Форматирует сообщение и отправляет на данный сокет.
```C
int tcp_printf(int socket, const char * format, ...);
```
Для отправки логов есть функция которая отправляет на сокет, указанный глобальной переменной `log_socket`. Для работы нужно подключить `log_socket` с помощью `tcp_connect()`, после чего приравнять `log_output` из `log.h` к `tcp_log_printf`.

```C
extern int log_socket;

int tcp_log_printf(const char * format, ...);

```

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