#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include "client.h"
#include <linux/limits.h>

typedef struct {
  client_t* client;
  char file_path[PATH_MAX];
} file_transfer_arg_t;

int open_data_transfer(client_t* client);
void close_data_transfer(client_t* client);
void* list_transfer(void* arg);
void* retr_transfer(void* arg);
void* stor_transfer(void* arg);
#endif
