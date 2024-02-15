#pragma once

#include "../conf.h"
#include "macro.h"
#include <stdbool.h>
#include <sys/socket.h>

#if CONF_IP_ENABLED

EXTERNC_BEGIN

#define PK_IP_ADDRSTRLEN INET_ADDRSTRLEN

// Общий хендлер для tcp/udp клиента/сервера.
typedef struct {
    // Локальный адрес сервера (в клиенте не используется)
    // Изменяется pk_tcp_server и pk_udp_server
    struct sockaddr_in local;
    // Удаленный адрес (сервера для клиента и наоборот)
    // В udp/tcp клиенте его нужно выставить через pk_set_remote или pk_set_remote_by_hostname
    // В udp сервере изменяется pk_udp_recv и pk_udp_recvn
    // В tcp сервере изменяется pk_tcp_accept
    struct sockaddr_in remote;
    // Используется только tcp сервером
    int ssfd;
    // Используется udp сервером и udp/tcp клиентом
    int csfd;
} pk_ip_hnd_t;

// Записывает ip адрес remote в виде строки в str
bool pk_ip_remote_addr2str(const pk_ip_hnd_t *hnd, char str[PK_IP_ADDRSTRLEN]);

// Создаёт tcp сокет, биндится, включает прослушивание
// hnd - указатель на хендлер, который будет использоваться следующими функциями
// backlog - количество подключений, которые будут ождать в очереди
// port - в машинном порядке
bool pk_tcp_server(pk_ip_hnd_t *hnd, uint16_t port, int backlog);
// Создаёт udp сокет, биндится
// hnd - указатель на хендлер, который будет использоваться следующими функциями
// backlog - количество подключений, которые будут ождать в очереди
// port - в машинном порядке
bool pk_udp_server(pk_ip_hnd_t *hnd, uint16_t port, int backlog);

// Создаёт tcp сокет
bool pk_tcp_client(pk_ip_hnd_t *hnd);
// Создаёт udp сокет
bool pk_udp_client(pk_ip_hnd_t *hnd);

// Устанавливает адрес и порт
// addr, port - в машинном порядке
void pk_ip_set_remote(pk_ip_hnd_t *hnd, uint32_t addr, uint16_t port);
// Устанавливает адрес по имени хоста и порт
// port - в машинном порядке
bool pk_ip_set_remote_by_hostname(pk_ip_hnd_t *hnd, const char *hostname, uint16_t port);

// Блокирует задачу до принятия запроса, адрес отправившего запрос записывается в remote.
bool pk_tcp_accept(pk_ip_hnd_t *hnd);

// Отправляет в tcp сокет buf размером n на адрес hnd->remote
// Отправка меньше n байтов будет считаться за ошибку
bool pk_tcp_send(const pk_ip_hnd_t *hnd, const void *buf, size_t n);
// Отправляет в udp сокет buf размером n на адрес hnd->remote
// Отправка меньше n байтов будет считаться за ошибку
bool pk_udp_send(pk_ip_hnd_t *hnd, const void *buf, size_t n);

// Блокируется до получения какого-то количества байт (не более max_n) из tcp сокета.
// Полученные байты записываются в buf.
// В отличие от других функций ip.h возвращает количество считаных байтов или -1 в случае ошибки.
size_t pk_tcp_recv(const pk_ip_hnd_t *hnd, void *buf, size_t max_n);
// Блокируется до получения какого-то количества байт (не более max_n) из udp сокета.
// В отличие от других функций ip.h возвращает количество считаных байтов или -1 в случае ошибки.
size_t pk_udp_recv(pk_ip_hnd_t *hnd, void *buf, size_t max_n);

// Блокируется до получения ровно n байтов из tcp сокета.
// Полученные байты записываются в buf.
// Получение меньше n байтов считается за ошибку.
bool pk_tcp_recvn(const pk_ip_hnd_t *hnd, void *buf, size_t n);
// Блокируется до получения ровно n байтов из udp сокета.
// Полученные байты записываются в buf.
// Получение меньше n байтов считается за ошибку.
bool pk_udp_recvn(pk_ip_hnd_t *hnd, void *buf, size_t n);

bool pk_ip_close_client(pk_ip_hnd_t *hnd);
#define pk_udp_close_server(hnd) pk_ip_close_client(hnd)

bool pk_tcp_close_server(pk_ip_hnd_t *hnd);

EXTERNC_END

#endif // CONF_IP_ENABLED
