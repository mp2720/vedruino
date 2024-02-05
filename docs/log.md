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