#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>

#define MAX_USERNAME 32
#define MAX_PASSWORD 64

void client_loop(int control_fd);
int client_connect(int control_fd);
void send_command(int fd, const char* cmd, const char* arg);
void send_line(int fd, char* line);
int recieve_line(int fd, char* buffer, size_t buffer_size);

#endif

