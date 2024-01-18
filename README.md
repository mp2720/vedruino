## Установка
Необходимы `make`, `arduino-cli`, `pyserial`.

Arch:
```bash
pacman -S make arduino-cli python-pyserial
```

## Конфиг
В файл `config` написать (значения переменных могут отличаться):
```makefile
PORT=/dev/ttyUSB0
FQBN=esp32:esp32:esp32da
```

FQBN можно посмотреть в выводе
```
arduino-cli board listall BOARD_NAME
```

## Доступ к устройству
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

## Добавление платы
```bash
make setup
```

