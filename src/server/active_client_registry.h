#ifndef ACTIVE_CLIENT_REGISTRY_H
#define ACTIVE_CLIENT_REGISTRY_H

#include <pthread.h>
#include "client.h"

#define START_CAPACITY 64

typedef struct {
  client_t** clients;
  size_t count;
  size_t capacity;
  pthread_mutex_t mutex;
} active_client_registry_t;

void init_client_registry(active_client_registry_t* self);
int client_registry_add(active_client_registry_t* self, client_t* client);
int client_registry_remove(active_client_registry_t* self, client_t* client);
void clear_client_registry(active_client_registry_t* self);

#endif
