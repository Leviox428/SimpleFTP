#include "client.h"
#include <asm-generic/errno.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

int start_server(int port_arg, char* root_folder_arg) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("Server process fork failed.\n");
    return -1;
  } else if (pid == 0) {
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port_arg);

    execl(
      "./server/server",   
      "server",            
      port_str,            
      root_folder_arg,     
      NULL
    );

    perror("Execl failed.\n");
    return - 2;
  } else {
    sleep(1);
    return 0;
  }
}

int main(int argc, char* argv[]) {
  int port = 2121;
  char* root_folder = "users";
  if (argc > 1) {
    port = atoi(argv[1]);
  }

  if (argc > 2) {
    root_folder = argv[2];
  }

  struct sockaddr_in server_addr;
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("Error creating a socket.\n");
    return 1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    perror("Invalid addres.\n");
    close(socket_fd);
    return 2;
  }

  if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    if (errno == ECONNREFUSED) {
      printf("Server not running. Starting server.\n");
      if (start_server(port, root_folder) == 0) {
        sleep(1);
        if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
          perror("Failed to connect after starting the server.\n");
          close(socket_fd);
          return 3;
        }
      } else {
        close(socket_fd);
        return 4;
      }
    } else {
      perror("Connection failed.\n");
      close(socket_fd);
      return 5;
    }
  }
  printf("Sucessfully connected to server on port %d.\n", port);

  if (!client_connect(socket_fd)) {
    printf("Server shutdown.\n");
    return 0;
  };
  client_loop(socket_fd);
  return 0;  

}
