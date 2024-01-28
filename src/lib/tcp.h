#ifndef D1I4S8T8LOG_H
#define D1I4S8T8LOG_H

#include <stddef.h>

//сокет на который отправляет tcp_log_printf()
extern int log_socket;

//подклчиться к серверу возвращает сокет или -1 при неудаче
int tcp_connect(const char * host_name, const char * host_port);

//отправить сообщение payload - данные, len - длинна, если len==0, то вычисляется длинна строки
int tcp_send(int socket, const char * payload, size_t len);

//закрыть сокет
int tcp_close(int socket);

//printf на удалённый сервер
int tcp_printf(int socket, const char * format, ...);

//printf на log_socket
int tcp_log_printf(const char * format, ...);

#endif
