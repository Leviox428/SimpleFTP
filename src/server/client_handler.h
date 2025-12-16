#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "client.h"
#include "active_client_registry.h"
#include "user_table.h"
#include <signal.h>

typedef struct {
  char* ftp_root;
  volatile sig_atomic_t* shutdown_requested;
  client_t* client;
  active_client_registry_t* client_registry;
  user_table_t* user_table;
} client_thread_arg_t;

void* handle_client(void* arg);
user_t* handle_connection(client_t* client, user_table_t* user_table, active_client_registry_t* registry);

#endif
