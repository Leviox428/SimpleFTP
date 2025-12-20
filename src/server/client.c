#include <linux/limits.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include "client.h"
#include "ftp_utils.h"
#include <errno.h>

int process_command(client_t* self, const char* buffer) {
  char cmd[16] = {0};
  char arg[496] = {0};
  
  sscanf(buffer, "%15s %495[^\r\n]", cmd, arg);

  if (!stricmp(cmd, "QUIT")) {
    send_response(self->socket_fd, "221", "Goodbye");
    return 1;
  }

  if (!stricmp(cmd, "PWD")) {
    char relative[512];
    get_user_cwd_relative(self, relative);
    send_response_fmt(self->socket_fd, "257", "\"%s\" is the current directory", relative);
    return 0;
  }

  if (!stricmp(cmd, "CWD")) {
    if (arg[0] == '\0') {
      send_response(self->socket_fd, "550", "CWD requires a directory name argument");
      return 0;
    }

    handle_cwd_command(self, arg);
    return 0;
  }

  if (!stricmp(cmd, "MKD")) {
    if (arg[0] == '\0') {
      send_response(self->socket_fd, "550", "MKD requires a directory name argument");
      return 0;
    }
    
    char new_dir_path[PATH_MAX];
    snprintf(new_dir_path, sizeof(new_dir_path), "%s/%s", self->cwd, arg);

    struct stat st = {0};
    if (stat(new_dir_path, &st) == -1) {
        if (mkdir(new_dir_path, 0755) == 0) {
            send_response_fmt(self->socket_fd, "257", "Directory %s created", arg);
        } else {
            perror("mkdir failed");
            send_response(self->socket_fd, "550", "Failed to create a new directory");

        }
    } else {
        send_response_fmt(self->socket_fd, "550", "Directory %s already exists", arg);
    }
    return 0;
  }

  if (!stricmp(cmd, "RMD")) {
    if (arg[0] == '\0') {
      send_response(self->socket_fd, "550", "RMD requires a directory name argument");
      return 0;
    } 
    handle_rmd_command(self, arg);
    return 0;
  }

  if (!stricmp(cmd, "DELE")) {
    if (arg[0] == '\0') {
      send_response(self->socket_fd, "550", "DELE requires a file name argument");
      return 0;
    }
    handle_dele_command(self, arg);
    return 0;
  }

  if (!stricmp(cmd, "PASV")) {
    handle_pasv_command(self);
    return 0;
  }
  
  if (!stricmp(cmd, "LIST")) {
    handle_list_command(self);
    return 0;
  }

  send_response(self->socket_fd, "550", "Unknown command");
  return 0;
}

void handle_rmd_command(client_t *self, char *arg) {
  char deleted_dir_path[PATH_MAX];
  snprintf(deleted_dir_path, sizeof(deleted_dir_path), "%s/%s", self->cwd, arg);

  struct stat st = {0};
  if (stat(deleted_dir_path, &st) == - 1) {
    send_response_fmt(self->socket_fd, "550", "Directory %s doesn't exist", arg);
    return;
  }

  if (!S_ISDIR(st.st_mode)) {
    send_response(self->socket_fd, "550", "Not a directory");
    return;
  }

  if (rmdir(deleted_dir_path) == -1) {
    if (errno == ENOTEMPTY || errno == EEXIST) {
      send_response(self->socket_fd, "550", "Directory not empty");
    } else {
      perror("Rmdir failed");
    }
    return;
  }
    
  send_response_fmt(self->socket_fd, "250", "Directory %s was removed", arg);
  return;
}

void handle_cwd_command(client_t *self, char* arg) {
  char candidate[PATH_MAX];
  char resolved[PATH_MAX];
  struct stat st;

  if (strcmp(arg, "/") == 0) {
      strncpy(self->cwd, self->home_dir, sizeof(self->cwd));
      self->cwd[sizeof(self->cwd) - 1] = '\0';
      send_response(self->socket_fd, "250", "Directory changed to /");
      return;
  }

  if (arg[0] == '/') {
    snprintf(candidate, sizeof(candidate), "%s%s", self->home_dir, arg);
  } else {     
    snprintf(candidate, sizeof(candidate), "%s/%s", self->cwd, arg);
  }

  if (!realpath(candidate, resolved)) {
      send_response(self->socket_fd, "550", "Failed to change directory");
      return;
  }

  if (strncmp(resolved, self->home_dir, strlen(self->home_dir)) != 0) {
      send_response(self->socket_fd, "550", "Permission denied");
      return;
  }

  if (stat(resolved, &st) < 0 || !S_ISDIR(st.st_mode)) {
      send_response(self->socket_fd, "550", "Not a directory");
      return;
  }

  strncpy(self->cwd, resolved, sizeof(self->cwd));
  self->cwd[sizeof(self->cwd) - 1] = '\0';

  send_response(self->socket_fd, "250", "Directory successfully changed");
}

void handle_dele_command(client_t *self, char *arg) {
  char deleted_file_path[PATH_MAX];
  snprintf(deleted_file_path, sizeof(deleted_file_path), "%s/%s", self->cwd, arg);
  
  if (unlink(deleted_file_path) == -1) {
    if (errno == ENOENT) {
      send_response_fmt(self->socket_fd, "550", "File %s doesn't exist", arg);
    } else if (errno == EACCES) {
      send_response(self->socket_fd, "550", "Permission denied");
    } else if (errno == EISDIR) {
      send_response(self->socket_fd, "550", "Tried to delete a folder use RMD instead");
    } else if (errno == EBUSY) {
      send_response(self->socket_fd, "550", "File in use");
    }
    return;
  }
  send_response_fmt(self->socket_fd, "250", "File %s was deleted", arg);
}

void handle_pasv_command(client_t *self) {
  pthread_mutex_lock(&self->mutex);
  if (self->pasv_fd != -1) {
    close(self->pasv_fd);
    self->pasv_fd = -1;
  }

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    send_response(self->socket_fd, "425", "Can't open passive connection");
    goto out;
  }

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr = self->ctrl_addr.sin_addr;
  addr.sin_port = 0;

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    send_response(self->socket_fd, "425", "Can't bind passive socket");
    goto out;
  }

  if (listen(fd, 1) < 0) {
    close(fd);
    send_response(self->socket_fd, "425", "Can't listen on passive socket");
    goto out;
  }

  socklen_t len = sizeof(addr);
  getsockname(fd, (struct sockaddr *)&addr, &len);

  uint16_t port = ntohs(addr.sin_port);
  uint32_t ip = ntohl(addr.sin_addr.s_addr);

  int p1 = port / 256;
  int p2 = port % 256;

  self->pasv_fd = fd;

  send_response_fmt(self->socket_fd, "227",
    "Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
    (ip >> 24) & 0xFF,
    (ip >> 16) & 0xFF,
    (ip >> 8)  & 0xFF,
    ip & 0xFF,
    p1, p2);

  out:
    pthread_mutex_unlock(&self->mutex);
}

void handle_list_command(client_t *self) {
}

void terminate_connection(client_t *self) {
  shutdown(self->socket_fd, SHUT_RDWR);
  close(self->socket_fd);
  free(self);
}

void get_user_cwd_relative(client_t *self, char *relative) {
  if (strcmp(self->cwd, self->home_dir) == 0) {
    strcpy(relative, "/");
    return;
  }

  const char* rel = self->cwd + strlen(self->home_dir);
  if (*rel == '\0') {
    strcpy(relative, "/");
    return;
  }
  strcpy(relative, rel);
}


