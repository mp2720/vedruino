#pragma once

#include "../conf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#if CONF_IP_ENABLED

typedef int pkSocketHandle_t;

typedef pkSocketHandle_t pkTcpHandle_t;
typedef pkTcpHandle_t pkTcpServer_t;
typedef pkTcpHandle_t pkTcpClient_t;

typedef pkSocketHandle_t pkUdpHandle_t;
typedef pkUdpHandle_t pkUdpServer_t;
typedef pkUdpHandle_t pkUdpClient_t;

#define PK_SOCKERR (-1)
// 16 for ip address, 1 for ':', 6 for 'port', 1 for zero
#define PK_IP_ADDR_STR_LEN (16 + 1 + 6 + 1)
#define PK_IP_BCAST_ADDR 0xffffffff

typedef struct pkIpAddr {
    uint32_t addr;
    uint16_t port;
} pkIpAddr_t;

void pk_ip_addr2str(pkIpAddr_t addr, char str[PK_IP_ADDR_STR_LEN]);

// Создаёт tcp сокет сервера, биндится, включает прослушивание.
// shd - указатель на хендлер сервера.
// port - в машинном порядке.
// backlog - количество подключений, которые будут ождать в очереди.
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pkTcpClient_t pk_tcp_server(uint16_t port, int backlog);
// Создаёт udp сокет сервера, биндится.
// shd - указатель на хендлер сервера.
// port - в машинном порядке.
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pkUdpHandle_t pk_udp_server(uint16_t port);

// Создаёт tcp сокет клиента.
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pkTcpClient_t pk_tcp_client();
// Создаёт udp сокет клиента
// Возвращает хендлер или, в случае неудачи, PK_SOCKERR.
pkUdpClient_t pk_udp_client();

// Подключает клиент chd к серверу с адресом srv_adr.
// Возвращает false только при ошибке.
bool pk_tcp_connect(pkTcpClient_t chd, pkIpAddr_t srv_addr);

// Блокирует задачу до принятия запроса.
// В out_cl_addr записывается адрес клиента (если не NULL)
pkTcpClient_t pk_tcp_accept(pkTcpServer_t shd, pkIpAddr_t *out_cl_addr);

// Отправляет через tcp сокет клиента буфер buf размером n
// Возвращает количество записанных байтов или -1 в случае ошибки.
ssize_t pk_tcp_send(pkTcpClient_t chd, const void *buf, size_t n);
// Отправляет по адресу addr через udp сокет буфер buf размером n
// Возвращает количество записанных байтов или -1 в случае ошибки.
ssize_t pk_udp_send(pkUdpHandle_t hd, pkIpAddr_t addr, const void *buf, size_t n);

// Отправляет через tcp сокет буфер buf размером n
// Отправка меньше n байтов будет считаться за ошибку
bool pk_tcp_sendn(pkTcpClient_t chd, const void *buf, size_t n);
// Отправляет по адресу addr через udp сокет буфер buf размером n
// Отправка меньше n байтов будет считаться за ошибку
bool pk_udp_sendn(pkUdpHandle_t hd, pkIpAddr_t addr, const void *buf, size_t n);

// Блокируется до получения какого-то количества байт (не более max_n) из tcp сокета.
// Полученные байты записываются в buf.
// Возвращает количество считаных байтов или -1 в случае ошибки.
ssize_t pk_tcp_recv(pkTcpClient_t chd, void *buf, size_t max_n);
// Блокируется до получения какого-то количества байт (не более max_n) из udp сокета.
// Адрес отправителя записывается в addr (если не NULL)
// Полученные байты записываются в buf.
// Возвращает количество считаных байтов или -1 в случае ошибки.
ssize_t pk_udp_recv(pkUdpHandle_t hd, pkIpAddr_t addr, void *buf, size_t max_n);

// Блокируется до получения ровно n байтов из tcp сокета.
// Полученные байты записываются в buf.
// Получение меньше n байтов считается за ошибку.
bool pk_tcp_recvn(pkTcpClient_t chd, void *buf, size_t n);
// Блокируется до получения ровно n байтов из udp сокета.
// Адрес отправителя записывается в addr (если не NULL)
// Полученные байты записываются в buf.
// Получение меньше n байтов считается за ошибку.
bool pk_udp_recvn(pkUdpHandle_t hd, pkIpAddr_t addr, void *buf, size_t n);

// Закрывает tcp/udp сокет.
bool pk_sock_close(pkSocketHandle_t hd);

#endif // CONF_IP_ENABLED
