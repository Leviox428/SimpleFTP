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
  char cmd[16];
  char arg[496];

  sscanf(buffer, "%15s %495[^\r\n]", cmd, arg);

  if (stricmp(cmd, "QUIT") == 0) {
    send_response(self->socket_fd, "502", "Goodbye");
    return 1;
  }

  if (stricmp(cmd, "PWD") == 0) {
    send_response_fmt(self->socket_fd, "257", "\"%s\" is the current directory", self->cwd);
    return 0;
  }

  if (stricmp(cmd, "CWD") == 0) {
    if (arg[0] == '\0') {
      send_response(self->socket_fd, "550", "CWD requires a directory argument");
      return 0;
    }

    handle_cwd_command(self, arg);
  }

  send_response(self->socket_fd, "550", "Unknown command");
  return 0;
}

void handle_cwd_command(client_t *self, char* arg) {
  char new_cwd[PATH_MAX];
  struct stat st;

  if (strcmp(arg, "/") == 0) {
    strncpy(self->cwd, self->home_dir, sizeof(self->cwd));
    send_response(self->socket_fd, "250", "Directory changed");
    return;
  }

  if (resolve_path(self, arg, new_cwd, sizeof(new_cwd)) < 0) {
    send_response(self->socket_fd, "550", "Invalid directory");
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


