#include "client.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int client_connect(int control_fd){
  char buffer[512];

  if (!recieve_line(control_fd, buffer, sizeof(buffer)))
    return 0;
  if (!recieve_line(control_fd, buffer, sizeof(buffer)))
    return 0;
  
  char username_buffer[MAX_USERNAME];
  while (1) {
    if (fgets(username_buffer, sizeof(username_buffer), stdin) == NULL) {
      printf("Entered username is empty, try again.\n");
      continue;
    }
    
    send_line(control_fd, username_buffer);
    if (!recieve_line(control_fd, buffer, sizeof(buffer)))
      return 0;

    if (strncmp("530", buffer, 3) == 0) 
      continue;
    
    break;
  }

  char password_buffer[MAX_PASSWORD];
  while (1) {
    if (fgets(password_buffer, sizeof(password_buffer), stdin) == NULL) {
      printf("Entered password is empty, try again.\n");
      continue;
    }

    send_line(control_fd, password_buffer);
    if (!recieve_line(control_fd, buffer, sizeof(buffer)))
      return 0;

    if (strncmp("530", buffer, 3) == 0) 
      continue;
    
    break;
  }
  return 1;
}

void client_loop(int control_fd) {
  
  char send_buffer[510];
  char receive_buffer[512];
  while (1) {
    printf("> ");
    if (fgets(send_buffer, sizeof(send_buffer), stdin) != NULL) {
      printf("%s", send_buffer);
      send_line(control_fd, send_buffer);
    }   
    if (!recieve_line(control_fd, receive_buffer, sizeof(receive_buffer)))
      return;
  }  
}

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
