#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include "client.h"
#include "ftp_utils.h"

int process_command(client_t* self, const char* buffer) {
  char cmd[16] = {0};
  char arg[496] = {0};
  
  sscanf(buffer, "%15s %495[^\r\n]", cmd, arg);

  if (!stricmp(cmd, "QUIT")) {
    send_response(self->socket_fd, "502", "Goodbye");
    return 1;
  }

  if (!stricmp(cmd, "PWD")) {
    send_response_fmt(self->socket_fd, "257", "\"%s\" is the current directory", self->cwd);
    return 0;
  }

  if (!stricmp(cmd, "CWD")) {
    if (arg[0] == '\0') {
      send_response(self->socket_fd, "550", "CWD requires a directory name argument");
      return 0;
    }

    handle_cwd_command(self, arg);
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

  send_response(self->socket_fd, "550", "Unknown command");
  return 0;
}

void handle_cwd_command(client_t *self, char* arg) {
  char new_cwd[PATH_MAX];
  struct stat st;

  if (strcmp(arg, "/") == 0) {
    strncpy(self->cwd, self->home_dir, sizeof(self->cwd));
    send_response(self->socket_fd, "250", "Directory changed to root");
    return;
  }


  if (stat(new_cwd, &st) < 0 || !S_ISDIR(st.st_mode)) {
    send_response(self->socket_fd, "550", "Not a directory");
    return;
  }

  strncpy(self->cwd, new_cwd, sizeof(self->cwd));
  self->cwd[sizeof(self->cwd) - 1] = '\0';

  send_response(self->socket_fd, "250", "Directory changed");
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


