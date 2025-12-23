#include "data_transfer.h"
#include "ftp_utils.h"
#include <linux/limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

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
  if (client->file_fd != -1) {
    close(client->file_fd);
    client->file_fd = -1;
  }
  pthread_mutex_unlock(&client->mutex);
  if (!client->abort_requested) {
    send_response(client->socket_fd, "226", "Closing data connection");
  } else {
    send_response(client->socket_fd, "426", "Transfer aborted");
    client->abort_requested = 0;
  }
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
    
    struct passwd pwbuf;
    struct passwd *pwresult;
    char pwname[256]; 

    if (getpwuid_r(st.st_uid, &pwbuf, pwname, sizeof(pwname), &pwresult) != 0 || !pwresult) {
      strcpy(pwname, "unknown");
    }

    struct group grbuf;
    struct group *grresult;
    char grname[256]; 

    if (getgrgid_r(st.st_gid, &grbuf, grname, sizeof(grname), &grresult) != 0 || !grresult) {
      strcpy(grname, "unknown");
    }

    snprintf(line, sizeof(line),
      "%s %3lu %-8s %-8s %8ld %s %s\r\n",
      perms,
      (unsigned long)st.st_nlink,
      pwname,
      grname, 
      (long)st.st_size,
      timebuf,
      ent->d_name);

      if (write(data_fd, line, strlen(line)) < 0) break;
  }
  closedir(dir);
  close_data_transfer(client);
  return NULL;
}

void* retr_transfer(void* arg) {
  file_transfer_arg_t* file_arg = (file_transfer_arg_t*)arg;

  int data_fd = open_data_transfer(file_arg->client);
  if (data_fd == -1) {
    send_response(file_arg->client->socket_fd, "550", "Couldnt start data transfer");
    goto cleanup;
  }

  char buf[8192];
  ssize_t n;
  
  while ((n = read(file_arg->client->file_fd, buf, sizeof(buf))) > 0) {
    if (file_arg->client->abort_requested) {
      break;
    }

    ssize_t sent = 0;
    while (sent < n) {
      if (file_arg->client->abort_requested) {
        break;
      }

      ssize_t w = write(data_fd, buf + sent, n - sent);
      if (w <= 0) {           
        goto cleanup;
      }

      sent += w;
    }
  }
  cleanup:
    close_data_transfer(file_arg->client);
    free(file_arg);
    return NULL;
}

void* stor_transfer(void* arg) {
  file_transfer_arg_t* file_arg = (file_transfer_arg_t*)arg;
  client_t* client = file_arg->client;

  int data_fd = open_data_transfer(client);
  if (data_fd == -1) {
    send_response(client->socket_fd, "550", "Couldn't start data transfer");
    goto cleanup;
  }

  char buf[8192];
  ssize_t n;

  while ((n = read(data_fd, buf, sizeof(buf))) > 0) {
    if (client->abort_requested) {
      break;
    }

    ssize_t written = 0;
    while (written < n) {
      if (client->abort_requested) {
        break;
      }

      ssize_t w = write(client->file_fd, buf + written, n - written);
      if (w <= 0) {
        goto cleanup;
      }

      written += w;
    }
  }

  cleanup:
    close_data_transfer(file_arg->client);
    free(file_arg);
    return NULL;
}

