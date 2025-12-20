#ifndef CLIENT_H
#define CLIENT_H

#include <linux/limits.h>
#include <stddef.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

typedef struct {
  int socket_fd;
  struct sockaddr_in ctrl_addr;
  
  time_t pasv_created;
  int pasv_fd;

  int data_fd;
  int logged_in;

  char cwd[PATH_MAX];
  char home_dir[PATH_MAX];

  int transfer_active;
  pthread_mutex_t mutex;

} client_t;

int process_command(client_t* self, const char* recieve_buffer);
void terminate_connection(client_t* self);
void handle_cwd_command(client_t* self, char* arg);
void handle_rmd_command(client_t* self, char* arg);
void handle_dele_command(client_t* self, char* arg);
void handle_list_command(client_t* self);
void handle_pasv_command(client_t* self);
void get_user_cwd_relative(client_t* self, char* relative);

#endif 
