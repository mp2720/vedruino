# netlog
Передаёт логи с платы на компьютер по протоколу TCP или UDP.

Использование TCP даёт большую надежность при передаче логов и позволяет обнаружить разрыв
соединения с платой (если keepalive не выключен), но увеличивает нагрузку на сеть и плату, а
некоторые сообщения могут приходить с значительной задержкой.

Клиент `tools/netlog.py` может продолжать работу (только если компьютер всё время работает в той
локальной сети, к которой подключается плата перед запуском `netlog`) даже после перезагрузок платы
за счёт автоматического переподключения после получения широковещательного оповещения о запуске
`netlog` сервера.

Для получения логов по TCP:
```bash
tools/netlog.py HOST -t
```

По UDP:
```bash
tools/netlog.py HOST
```

`HOST` - IP адрес или имя хоста (можно использовать mDNS на Linux, если avahi включён).

При использовании TCP можно отключить keepalive флагом `-k`.

## Конфигурация
```ini
[lib.netlog]
enabled=true
max_clients=3
buf_size=255
```

* `max_clients` - количество клиентов, подписанных на рассылку.
* `buf_size` - размер буфера для stdout и размер буфера, в который копируются данные из stdout перед
  отправкой по UDP.

## Протокол
На плате работает сервер контроля рассылки, который принимает TCP запросы на порту `8419` для
подписки или отписки от рассылки логов.

Сервер хранит у себя ограниченный по размеру список подписанных клиентов.

Клиент должен подключиться к этому порту через TCP сокет, отправить запрос на подписку, в котором
указывается протокол, который будет использован для рассылки, а также номер порта на своём узле,
на который сервер будет отправлять логи.

Сервер принимает запрос и возвращает ответ:
* статус `+` - всё в порядке, далее идёт информация о плате (MAC адрес и номер сессии).
* статус `-` - произошла ошибка, далее идёт её описание, после чего TCP сокет закрывается.

Дальнейшее взаимодействие определяется протоколом рассылки:
* UDP: сервер добавляет клиента в список и закрывает TCP сокет. Логи отправляются на UDP порт узла
  клиента.
* TCP: сервер добавляет клиента в список и закрывает TCP сокет. Затем сервер подключается к TCP
  порту клиента, куда далее отправляются логи.

Если клиент завершает свою работу, то он должен отправить запрос на отписку, после чего закрыть
сокеты.

Если протокол рассылки TCP, то при выключении платы соединение не разрывается, но никакие логи по
нему не отправляются (даже если плата после этого была вновь включена и netlog сервер запустился).
Клиент может использовать keepalive для проверки того, что соединение продолжает использоваться
сервером (но это увеличивает нагрузку на сеть и, что гораздо важнее, на плату).

Протокол UDP не подразумевает установку и поддержку соединение, поэтому o выключении платы узнать
никак нельзя.

Чтобы netlog было удобнее использовать, реализован механизм широковещательных оповещений о запуске
сервера. Клиент должен ожидать их на UDP порту `8419`. Сервер отправляет несколько пакетов для
уменьшения вероятности потери оповещения. Каждый такой пакет содержит MAC адрес платы, по которому
клиент отделяет оповещения нужной ему платы от остальных. Поскольку пакетов несколько, а UDP не
предоставляет возможность подтвердить их доставку, как и порядок их принятия, каждый пакет также
содержит 256-битный номер сессии, который сервер случайно генерирует во время своего запуска. Клиент
должен, пользуясь номером сессии, определять, действительно ли сервер был перезапущен с момента
последней подписки.

Номер сессии в оповещении о перезапуске совпадает с номером сессии, который возвращает сервер в
ответ на следующий после оповещения TCP запрос (если между этими событиями больше не было
перезапусков).

Хотя случайная генерация номера сессии и не гарантирует его уникальность, шанс совпадения двух
случайных 256-битных чисел принебрежимо мал.

### Формат данных
TCP запросы к серверу для контроля рассылки имеет следующие форматы, где `{port}` - номер UDP
порта в виде 2 байтов в сетевом порядке, на который сервер отсылает логи:
* `2720NETLOG{proto}{port}S` - для подписки.
* `2720NETLOGUDP{proto}{port}U` - для отписки.

Ответ сервера на TCP запрос контроля рассылки может иметь один из двух видов:
* `+{mac}{session}` - запрос выполнен успешно, `{mac}` - MAC адрес платы (6 байт), `{session}` -
   номер сессии (32 байта).
* `-{msg}` - ошибка, `{msg}` - сообщение об ошибке в текстовом виде.

Размер успешного ответа - 39 байт, а размер ответа об ошибке не ограничен в размере - клиент должен
считывать его до закрытия сокета.

Пакеты широковещательного оповещения выглядят так: `2720NETLOG{mac}{session}`, `{mac}` и `{session}`
означают то же, что и в ответе сервера на запрос.

При рассылке логов по TCP сообщения передаются непрерывным текстовым потоком. Отделять одно
сообщение от другого можно по символу новой строки.

При рассылке логов по UDP каждое сообщение отправляется в виде текста, который обычно оканчивается
символом новой строки, в отдельном пакете. Кроме того, первый байт пакета содержит 7-битный
последовательный номер пакета, который клиент использует для сообщения пользователю о неправильном
порядке передачи (может означать и потерю пакетов).

## Реализация сервера
Сервер работает в отдельной задаче `pknetlog_ctl` с приоритетом 10 и 4KiB стеком.

При инциализации `stdout` подменяется в текущей задаче и в global reent, то есть у всех задач,
которые будут запущены после этого, `stdout` также будет подменён.

В подменённом `stdout` включена буферизация по строкам, размер буфера задаётся в конфигурации.
Отправка данных из буфера происходит в задаче, из которой был вызвана последняя функция вывода в
`stdout` (например `printf()`). Эта задача блокируется до завершения отправки, а стек подменяется,
поэтому для других задач нужен стек не больше того, что они используют без netlog. Отправка защищена
мьютексом.

Вся память выделяется статически.
