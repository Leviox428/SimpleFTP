#include "data_transfer.h"
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

int open_data_transfer(client_t* client) {
  pthread_mutex_lock(&client->mutex);
  int pasv_fd = client->pasv_fd;
  client->pasv_fd = -1;
  pthread_mutex_unlock(&client->mutex);

  if (pasv_fd < 0) return -1;
  

  int data_fd = accept(pasv_fd, NULL, NULL);
  close(pasv_fd);

  if (data_fd < 0) return -1;

  pthread_mutex_lock(&client->mutex);
  client->data_fd = data_fd;
  pthread_mutex_unlock(&client->mutex);

  return data_fd;  
}

void close_data_transfer(client_t *client) {
  pthread_mutex_lock(&client->mutex);
  if (client->data_fd != -1) {
      close(client->data_fd);
      client->data_fd = -1;
  }

  client->transfer_active = 0;
  pthread_mutex_unlock(&client->mutex);
}
