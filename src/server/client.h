#ifndef CLIENT_H
#define CLIENT_H

#include <linux/limits.h>
#include <stddef.h>
#include <pthread.h>

typedef struct {
  int socket_fd;
  int data_fd;
  int logged_in;

  char cwd[PATH_MAX];
  char home_dir[PATH_MAX];

  pthread_t data_thread;
  pthread_mutex_t mutex;
  volatile int transfer_active;

} client_t;

int process_command(client_t* self, const char* recieve_buffer);
void terminate_connection(client_t* self);
void handle_cwd_command(client_t* self, char* arg);
void handle_rmd_command(client_t* self, char* arg);
void handle_dele_command(client_t* self, char* arg);
void get_user_cwd_relative(client_t* self, char* relative);

#endif 
