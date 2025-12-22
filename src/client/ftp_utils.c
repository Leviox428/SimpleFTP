#include "ftp_utils.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int recieve_line(int fd, char *buffer, size_t buffer_size) {
  size_t total = 0;
  char c;

  while (total < buffer_size - 1) {
      ssize_t n = recv(fd, &c, 1, 0);
      if (n <= 0) return 0; 
      if (c == '\n') break;
      if (c != '\r') buffer[total++] = c;
  }

  buffer[total] = '\0';
  printf("%s\n", buffer);
  return 1;
}

void send_line(int fd, char *line) {
  line[strcspn(line, "\n")] = '\0';
  char buffer[512];

  snprintf(buffer, sizeof(buffer), "%s\r\n", line);
  send(fd, buffer, strlen(buffer), 0);
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
