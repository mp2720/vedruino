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

