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
Там всё должно работать нормально, но будь человеком, не пользуйся этим говном.

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
<summary>Если `adbc` не работает</summary>
Если с `adbc` будут какие-то проблемы, то можно достать `compile_commands.json` из
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

