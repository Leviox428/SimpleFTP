#include <linux/limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ftp_utils.h"
#include <sys/socket.h>

void send_response(int socket_fd, const char *code, const char *response) {
  char buffer[CONTROL_BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer), "%s %s\r\n", code, response);
  send(socket_fd, buffer, strlen(buffer), 0);
}

void send_response_fmt(int socket_fd, const char *code, const char *fmt, ...) {
  char buffer[CONTROL_BUFFER_SIZE];
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

void format_permissions(mode_t mode, char *permissions) {
  permissions[0] = S_ISDIR(mode) ? 'd' :
           S_ISLNK(mode) ? 'l' : '-';

  permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
  permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
  permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
  permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
  permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
  permissions[6] = (mode & S_IXGRP) ? 'x' : '-';
  permissions[7] = (mode & S_IROTH) ? 'r' : '-';
  permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
  permissions[9] = (mode & S_IXOTH) ? 'x' : '-';
  permissions[10] = '\0';
}

