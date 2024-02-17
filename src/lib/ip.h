#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#if CONF_IP_ENABLED

EXTERNC_BEGIN

typedef int pk_sockhd_t;

typedef pk_sockhd_t pk_tcphd_t;
typedef pk_tcphd_t pk_tcp_srvhd_t;
typedef pk_tcphd_t pk_tcp_clhd_t;

typedef pk_sockhd_t pk_udphd_t;
typedef pk_udphd_t pk_udp_srvhd_t;
typedef pk_udphd_t pk_udp_clhd_t;

#define PK_SOCKERR (-1)
// 16 for ip address, 1 for ':', 6 for 'port', 1 for zero
#define PK_IP_ADDR_STR_LEN (16 + 1 + 6 + 1)

typedef struct {
    uint32_t addr;
    uint16_t port;
} pk_ip_addr;

void pk_ip_addr2str(pk_ip_addr addr, char str[PK_IP_ADDR_STR_LEN]);

// Создаёт tcp сокет сервера, биндится, включает прослушивание.
// shd - указатель на хендлер сервера.
// port - в машинном порядке.
// backlog - количество подключений, которые будут ождать в очереди.
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pk_tcp_clhd_t pk_tcp_srv(uint16_t port, int backlog);
// Создаёт udp сокет сервера, биндится.
// shd - указатель на хендлер сервера.
// port - в машинном порядке.
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pk_udphd_t pk_udp_srv(uint16_t port);

// Создаёт tcp сокет клиента.
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pk_tcp_clhd_t pk_tcp_cl();
// Создаёт udp сокет клиента
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pk_udp_clhd_t pk_udp_cl();

// Подключает клиент chd к серверу с адресом srv_adr.
// Возвращает false только при ошибке.
bool pk_tcp_connect(pk_tcp_clhd_t chd, pk_ip_addr srv_addr);

// Блокирует задачу до принятия запроса.
// В out_cl_addr записывается адрес клиента (если не NULL)
pk_tcp_clhd_t pk_tcp_accept(pk_tcp_srvhd_t shd, pk_ip_addr *out_cl_addr);

// Отправляет через tcp сокет клиента буфер buf размером n
// Возвращает количество записанных байтов или -1 в случае ошибки.
ssize_t pk_tcp_send(pk_tcp_clhd_t chd, const void *buf, size_t n);
// Отправляет по адресу addr через udp сокет буфер buf размером n
// Возвращает количество записанных байтов или -1 в случае ошибки.
ssize_t pk_udp_send(pk_udphd_t hd, pk_ip_addr addr, const void *buf, size_t n);

// Отправляет через tcp сокет буфер buf размером n
// Отправка меньше n байтов будет считаться за ошибку
bool pk_tcp_sendn(pk_tcp_clhd_t chd, const void *buf, size_t n);
// Отправляет по адресу addr через udp сокет буфер buf размером n
// Отправка меньше n байтов будет считаться за ошибку
bool pk_udp_sendn(pk_udphd_t hd, pk_ip_addr addr, const void *buf, size_t n);

// Блокируется до получения какого-то количества байт (не более max_n) из tcp сокета.
// Полученные байты записываются в buf.
// Возвращает количество считаных байтов или -1 в случае ошибки.
ssize_t pk_tcp_recv(pk_tcp_clhd_t chd, void *buf, size_t max_n);
// Блокируется до получения какого-то количества байт (не более max_n) из udp сокета.
// Адрес отправителя записывается в addr (если не NULL)
// Полученные байты записываются в buf.
// Возвращает количество считаных байтов или -1 в случае ошибки.
ssize_t pk_udp_recv(pk_udphd_t hd, pk_ip_addr addr, void *buf, size_t max_n);

// Блокируется до получения ровно n байтов из tcp сокета.
// Полученные байты записываются в buf.
// Получение меньше n байтов считается за ошибку.
bool pk_tcp_recvn(pk_tcp_clhd_t chd, void *buf, size_t n);
// Блокируется до получения ровно n байтов из udp сокета.
// Адрес отправителя записывается в addr (если не NULL)
// Полученные байты записываются в buf.
// Получение меньше n байтов считается за ошибку.
bool pk_udp_recvn(pk_udphd_t hd, pk_ip_addr addr, void *buf, size_t n);

// Закрывает tcp/udp сокет.
bool pk_sock_close(pk_sockhd_t hd);

EXTERNC_END

#endif // CONF_IP_ENABLED
