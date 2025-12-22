#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>
#include <pthread.h>

#define MAX_USERNAME 32
#define MAX_PASSWORD 64

typedef struct {
  int control_fd;
  int data_fd;
  int transfer_active;
  int output_fd;
  pthread_t data_thread;
} client_t;

void client_loop(int control_fd);
int client_connect(int control_fd);
void send_command(int fd, const char* cmd, const char* arg);
int handle_227(client_t* client, const char* line);
void *data_receive_thread(void *arg);
void close_data_connection(client_t* self);
#endif

