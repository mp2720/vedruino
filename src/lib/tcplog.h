#pragma once

#include "macro.h"
#include <stddef.h>
#include <esp_err.h>

EXTERNC_BEGIN
// сокет на который отправляет tcp_log_printf()
extern int log_socket;

// подклчиться к серверу возвращает сокет или -1 при неудаче
int tcplog_connect(const char *host_name, const char *host_port);

// отправить сообщение payload - данные, len - длинна, если len==0, то вычисляется длинна строки
/* int tcplog_send(int socket, const char *payload, size_t len); */

// закрыть сокет
/* int tcplog_close(int socket); */

// printf на удалённый сервер
/* int tcplog_sock_printf(int socket, const char *format, ...); */

// printf на log_socket
int tcplog_printf(const char *format, ...);
EXTERNC_END
