#ifndef FTP_UTILS_H
#define FTP_UTILS_H

#include "client.h"
#include <stddef.h>

#define CONTROL_BUFFER_SIZE 512

void send_response(int socket_fd, const char* code, const char* response);
void send_response_fmt(int sockfd, const char *code, const char *fmt, ...);
int receive_line(client_t* client, char* buffer, size_t buffer_size);
int stricmp(const char* a, const char* b);
int strincmp(const char* a, const char*b, size_t n);
int resolve_path(client_t* client, const char* arg, char* out, size_t out_size);

#endif
