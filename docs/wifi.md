# WIFI
Для подключения к сети использовать данную функцию, она берёт значения из конфига и возвращает __1__ в случае успеха и __0__ при неудаче

```C
bool pk_wifi_connect();
```

Конфиг:
```ini
[lib.wifi]
enabled=(bool)
ssid=(str)
password=(str)
retry=(int)
```
