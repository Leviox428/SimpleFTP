#include "data_transfer.h"
#include "ftp_utils.h"
#include <linux/limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int open_data_transfer(client_t* client) {
  pthread_mutex_lock(&client->mutex);
  int pasv_fd = client->pasv_fd;
  client->pasv_fd = -1;
  pthread_mutex_unlock(&client->mutex);

  if (pasv_fd < 0) return -1;
  

  int data_fd = accept(pasv_fd, NULL, NULL);
  close(pasv_fd);

  if (data_fd < 0) return -1;

  client->data_fd = data_fd;
  
  send_response(client->socket_fd, "150", "Starting data transfer");

  return data_fd;  
}

void close_data_transfer(client_t *client) {
  if (client->data_fd != -1) {
      close(client->data_fd);
      client->data_fd = -1;
  }

  pthread_mutex_lock(&client->mutex);
  client->transfer_active = 0;
  pthread_mutex_unlock(&client->mutex);
 
  send_response(client->socket_fd, "226", "Closing data connection");
}

void* list_transfer(void* arg) {
  client_t* client = (client_t*)arg;

  int data_fd = open_data_transfer(client);
  if (data_fd == -1) {
    send_response(client->socket_fd, "550", "Couldnt start data transfer");
    pthread_mutex_lock(&client->mutex);
    client->transfer_active = 0;
    pthread_mutex_unlock(&client->mutex);
    return NULL;
  }
  
  char dir_path[PATH_MAX];

  pthread_mutex_lock(&client->mutex);
  snprintf(dir_path, sizeof(dir_path), "%s", client->cwd);
  pthread_mutex_unlock(&client->mutex);

  DIR* dir = opendir(dir_path);
  if (!dir) {
    send_response(client->socket_fd, "550", "Couldn't open dir for listing");
    return NULL;
  }

  struct dirent *ent;
  char fullpath[PATH_MAX];
  char line[CONTROL_BUFFER_SIZE];

  while ((ent = readdir(dir)) != NULL) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;
    
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, ent->d_name);

    struct stat st;
    if (lstat(fullpath, &st) < 0) continue;

    char perms[11];
    format_permissions(st.st_mode, perms);

    struct tm tm;
    localtime_r(&st.st_mtime, &tm);

    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", &tm);

    snprintf(line, sizeof(line),
      "%s %3lu %-8s %-8s %8ld %s %s\r\n",
      perms,
      (unsigned long)st.st_nlink,
      "user", "group",   
      (long)st.st_size,
      timebuf,
      ent->d_name);

      if (write(data_fd, line, strlen(line)) < 0) break;
  }
  closedir(dir);
  close_data_transfer(client);
  return NULL;
}

