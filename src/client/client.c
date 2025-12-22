#include "client.h"
#include "ftp_utils.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>         
#include <arpa/inet.h>      
#include <netinet/in.h>

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
    if (!recieve_line(control_fd, buffer, sizeof(buffer))) return 0;

    if (strncmp("530", buffer, 3) == 0) continue;
    
    break;
  }
  return 1;
}

void client_loop(int control_fd) {
  
  client_t client = {0};
  client.data_fd = -1;
  client.transfer_active = 0;
  client.control_fd = control_fd;
  
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

    if (strncmp(receive_buffer, "227", 3) == 0) {
      handle_227(&client, receive_buffer);
    }

    if (strncmp(receive_buffer, "150", 3) == 0) {
      client.transfer_active = 1;
      
      if (strincmp("LIST", send_buffer, 4) == 0) {
        client.output_fd = STDOUT_FILENO;
        pthread_create(&client.data_thread, NULL, data_receive_thread, &client);
        pthread_detach(client.data_thread);
      }

      if (strincmp("RETR", send_buffer, 4) == 0) {
      }
      
      if (strincmp("STOR", send_buffer, 4) == 0) {
      }
    }
  }  
}

int handle_227(client_t *client, const char *line) {
  int h1,h2,h3,h4,p1,p2;

  sscanf(line,
    "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
    &h1,&h2,&h3,&h4,&p1,&p2);

  int port = p1 * 256 + p2;

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port)
  };

  char ip[32];
  snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1,h2,h3,h4);
  inet_pton(AF_INET, ip, &addr.sin_addr);

  client->data_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client->data_fd < 0) {
    perror("Could not create data transfer socket");
    return -1;
  }

  if (connect(client->data_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Could not connect to server");
    close(client->data_fd);
    client->data_fd = -1;
    return -1;
  }
  return 0;
}

void *data_receive_thread(void *arg) {
  client_t* client = (client_t*)arg;
  char buffer[512];
  ssize_t n;

  printf("\n");
  fflush(stdout);

  while ((n = read(client->data_fd, buffer, sizeof(buffer))) > 0) {
      write(client->output_fd, buffer, n);
  }

  if (client->output_fd != STDOUT_FILENO) close(client->output_fd);
 
  close(client->data_fd);

  client->data_fd = -1;
  client->transfer_active = 0;
  recieve_line(client->control_fd, buffer, sizeof(buffer));
  return NULL;
}

