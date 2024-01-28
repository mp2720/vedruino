`comp_coms_halalizer.py` - для халялизации `compile_commands.json`.

`vedruino.ino` - без него `arduino-cli` не может.

`src/` - весь код здесь и никаких `.ino`, `.pde`.

# Установка
Необходимы `make`, `arduino-cli`, `pyserial`.

Arch:
```bash
pacman -S make arduino-cli python-pyserial
```

# Конфиг
В файл `config` написать (значения переменных могут отличаться):
```makefile
PORT=/dev/ttyUSB0
FQBN=esp32:esp32:esp32da
```

FQBN можно посмотреть в выводе
```
arduino-cli board listall BOARD_NAME
```

<details>
<summary>Про `sketch.yaml`</summary>
`sketch.yaml` будет сгенерирован автоматически после добавления платы и будет содержать те же поля,
что и `config`, но, эти значения умеют доставать из `sketch.yaml` не все команды `arduino-cli` и не
всегда, поэтому они передаются явно в `makefile`.
</details>

# Доступ к устройству
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

# Добавление платы
```bash
make setup
```

# Arduino IDE 1/2
Это полное говно, лучше хотя бы vscode.

# LSP
Сначала нужно добавить плату.

Можно использовать `clangd` или `arduino-language-server` (обёртка над `clangd`).
Первый вариант имеет следующие преимущества:

* Намного быстрее запускается и реагирует на изменения в коде.
* Не игнорирует `.clang-format` и (наверное) `.clangd`.
* Можно поменять параметры комплиятора, например включить предупреждения (которые в
  `arduino-language-server` отключены и способа это изменить разработчики не предусмотрели).
* С `arduino-language-server` по какой-то причине хуже работает `nvim` и его плагины.

В остальном всё примерно одинаково.

Тем не менее, вероятность того, что `arduino-language-server` будет работать хоть как-то больше.

## clangd
Необходимо установить `acdb`:
```bash
git clone https://github.com/mp1884/acdb
mkdir ./acdb/build && cd ./acdb/build
cmake -S.. -B. -DCMAKE_INSTALL_PREFIX=~/.local
make install
```

Затем в директории этого репозитория обновить `compile_commands.json`:
```bash
make updcc
```

<details>
<summary>Если `acdb` не работает</summary>
Если с `acdb` будут какие-то проблемы, то можно достать `compile_commands.json` из
`/tmp/arduino/sketches/%SKETCH_ID%/` после выполнения команды:

```bash
arduino-cli compile --only-compilation-database
```

Где взять `%SKETCH_ID%` - непонятно. Затем нужно запускать `python comp_coms_halalizer.py`.
</details>

Эту же команду нужно выполнять после добавления каждого `.cpp` (возможно и `.h`) файла .

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

Параметры компилятора можно передавать через `EXTRA_ARGS` в `comp_coms_halalizer.py`.

## [arduino-language-server](https://github.com/arduino/arduino-language-server)
Нужно сгенерировать конфиг, если его еще нет:
```bash
arduino-cli config init
```

Дальше всё понятно.



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

### LIBLOG_LEVEL 
Макрос уровня логгирования, можно установить самостоятельно до 
`#include "lib/log.h"`, по умолчанию равен 5

* __0__ - нет логов
* __1__ - error
* __2__ - warning
* __3__ - info
* __4__ - debug
* __5__ - verbose

Чем больше уровень, тем менее значимые сообщения будут приходить
```C
#ifndef LIBLOG_LEVEL
    #define LIBLOG_LEVEL 5
#endif
```

### log_output
Указатель на функцию с printf-подобным интерфейсом, по умолчанию ему и равен.
При изменении на `tcp_log_printf` логи будут передаваться новой функции
```C
typedef int (*printf_like_t) (const char *, ...);

extern printf_like_t log_output; 
```

# lib/tcp.h
Набор функций для работы с сокетами

## connect
Функция получает имя сервера и порт, пытается подключиться, и при успехе возвращает дескриптор сокета, который используется в дальнейшем, иначе возвращает -1.

Именем хоста может быть IP адрес или домен

Портом может быть число или название сервиса (см. /etc/services)
```C
int tcp_connect(const char * host_name, const char * host_port);
```
## close
Получает сокет, и отключает его от сервера. Обязательно использовать перед перезаписью дескриптора сокета
```C
int tcp_close(int socket);
```

## send
Отправляет сообщение `payload` на данный сокет. В `len` нужно указать длинну сообщения в байтах, если это строка, то можно оставить `len` равным 0, тогда длина будет вычислена с помощью `strlen()`
```C
int tcp_send(int socket, const char * payload, size_t len);
```

## printf
Форматирует сообщение и отправляет на данный сокет.
```C
int tcp_printf(int socket, const char * format, ...);
```
Для отправки логов есть функция которая отправляет на сокет, указанный глобальной переменной log_socket. Для работы нужно подключить `log_socket` с помощью `tcp_connect()`, после чего приравнять `log_output` из `log.h` к `tcp_log_printf`.

```C
extern int log_socket;

int tcp_log_printf(const char * format, ...);

```

# lib/mqtt.h

Библиотека для удобной работы с mqtt. Подписывается на данный массив топиков и вызывает функции при получении сообщений

## init
Инициализирует задачу и очередь. Вызывать в первую очередь, единожды.
```C
void mqtt_init();
```

## connect
Подключение к брокеру
* `broker_host` - имя хоста
* `broker_port` - порт хоста
* `username` - имя пользователя mqtt
* `password` - пароль пользователя mqtt

Возвращает __0__ при успехе и __1__ при неудаче

```C
int mqtt_connect(const char * broker_host, uint16_t broker_port, const char * username, const char * password);
```
## subscribe

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

Возвращает __0__ при успехе и __1__ при неудаче

```C
int mqtt_subscribe_topics(fl_topic_t topics[], int len); 
```

## unsubscribe
Отписывается от топика с данным именем

Возвращает __0__ при успехе и __1__ при неудаче
```C
int mqtt_unsubscribe_topic(const char * name);
```

## publish
Функция отправки сообщения
* `topic` - имя топика
* `data` - указатель на данные
* `data_size` - размер данных, если строка, то можно указать 0, и размер будет вычислен strlen()
* `qos` - quality of service
* `retain` - флаг брокеру, отправить это сообщение новым устройствам при их подключении

Возвращает __0__ при успехе и __1__ при неудаче
```C
void mqtt_publish(const char * topic, const char * data, size_t data_size, int qos, bool retain); 
```
## disconnect
Отписывается от текущего брокера
```C
int mqtt_disconnect(); 
```

## stop/resume
Останавливает и запускает вновь работу mqtt
```C
void mqtt_stop(); 
void mqtt_resume(); 
```
## check
Возвращает `true` если брокер подключён, иначе `false`. Можно использовать для ожидания подключения. 
```C
bool mqtt_is_connected(); 
```