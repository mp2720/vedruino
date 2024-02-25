# mDNS
Перед включением mDNS на плате нужно записать её hostname в nvs.
Flash память должна быть размечена как в `partitions.csv`, это можно сделать через `make flash`.
Затем нужно установить esp-idf в `ESP_IDF_PATH` и запустить `tools/flash_hostname.sh`:
```bash
tools/flash_hostname.sh ESP_IDF_PATH HOSTNAME SERIAL_PORT
```

`HOSTNAME` - hostname платы без `.local` (добавится автоматически).

## Конфигурация
```ini
[lib.mdns]
enabled=true
```

