#include <linux/limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ftp_utils.h"
#include <sys/socket.h>

void send_response(int socket_fd, const char *code, const char *response) {
  char buffer[512];
  snprintf(buffer, sizeof(buffer), "%s %s\r\n", code, response);
  send(socket_fd, buffer, strlen(buffer), 0);
}

void send_response_fmt(int socket_fd, const char *code, const char *fmt, ...) {
  char buffer[512];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  send_response(socket_fd, code, buffer);
}

int receive_line(client_t* client, char* buffer, size_t buffer_size) {
  size_t total = 0;
  char c;

  while (total < buffer_size - 1) {
      ssize_t n = recv(client->socket_fd, &c, 1, 0);
      if (n <= 0) return 0;
      if (c == '\n') break; 
      if (c != '\r') buffer[total++] = c; 
  }

  buffer[total] = '\0';
  printf("%s\n", buffer);
  return 1;
}

int stricmp(const char* a, const char *b) {
  while (*a && *b) {
    char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
    char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
    if (ca != cb) return ca - cb;
    a++; b++;
  }
  return *a - *b;
}

int strincmp(const char *a, const char *b, size_t n) {
  size_t i = 0;
  while (i < n && *a && *b) {
    char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
    char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
    if (ca != cb) return ca - cb;
    a++; b++; i++;
  }
  if (i < n) {
    return *a - *b;
  }
  return 0;
}

int resolve_path(client_t *client, const char *arg, char *out, size_t out_size) {
  char tmp[PATH_MAX];

  if (arg[0] == '/') {
    snprintf(tmp, sizeof(tmp), "%s%s", client->home_dir, arg);
  } else {
    snprintf(tmp, sizeof(tmp), "%s/%s", client->cwd, arg);
  }

  if (!realpath(tmp, out)) {
    return -1;
  }
  
  // ensure path stays in home_dir
  if (strncmp(out, client->home_dir, strlen(client->home_dir)) != 0) {
    return -1;
  }

  return 0;
}

