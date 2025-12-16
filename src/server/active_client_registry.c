#include "active_client_registry.h"
#include "client.h"
#include "ftp_utils.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void init_client_registry(active_client_registry_t *self) {
  self->clients = malloc(sizeof(client_t*) * START_CAPACITY);
  self->count = 0;
  self->capacity = START_CAPACITY;
  pthread_mutex_init(&self->mutex, NULL);
}

int client_registry_add(active_client_registry_t *self, client_t *client) {
  pthread_mutex_lock(&self->mutex);
  
  int return_message = 1;
  if (self->count == self->capacity) {
    self->capacity *= 2;
    self->clients = realloc(self->clients, self->capacity * sizeof(client_t*));
    return_message = 0;
  }

  self->clients[self->count++] = client;
  pthread_mutex_unlock(&self->mutex);
  return return_message;
}

int client_registry_remove(active_client_registry_t *self, client_t* client) {
  pthread_mutex_lock(&self->mutex);
  
  for (size_t i = 0; i < self->count; i++) {
    if (self->clients[i] == client) {
      self->clients[i] = self->clients[--self->count];
      pthread_mutex_unlock(&self->mutex);
      close(client->socket_fd);
      free(client);
      return 1;
    }
  }
  pthread_mutex_unlock(&self->mutex);
  return 0;
}

void clear_client_registry(active_client_registry_t *self) {
  pthread_mutex_lock(&self->mutex);
  
  for (size_t i = 0; i < self->count; i++) {
    client_t* client = self->clients[i];
    
    if (client) {
      send_response(client->socket_fd, "421", "Server shutting down");
      terminate_connection(client);
    }
  }

  pthread_mutex_unlock(&self->mutex);
  pthread_mutex_destroy(&self->mutex);
}
