#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "active_client_registry.h"
#include "client_handler.h"
#include "console_manager.h"
#include "user_table.h"
#include <pthread.h>
#include <pwd.h>
#include <stdatomic.h>

volatile sig_atomic_t shutdown_requested = 0;

void handle_sigint(int sig) {
  printf("Caught SIGINT, closing the server\n");
  shutdown_requested = 1;
} 

void* scan_clients(void* arg) {
  active_client_registry_t* client_registry = (active_client_registry_t*)arg;
  if (!client_registry) return NULL;
  while (!shutdown_requested) {
    sleep(20);
    time_t now = time(NULL);
    pthread_mutex_lock(&client_registry->mutex);
    for (int i = 0; i < client_registry->count; i++) {
      client_t* client = client_registry->clients[i];
      pthread_mutex_lock(&client->mutex);
      if (client->pasv_fd != -1 &&
        difftime(now, client->pasv_created) > 60) {        
        close(client->pasv_fd);
        client->pasv_fd = -1;
      }
      pthread_mutex_unlock(&client->mutex);
    }
    pthread_mutex_unlock(&client_registry->mutex);
  }
  return NULL;
}

int main(int argc, char* argv[]) {
  int port = 2121;
  char* root = "users";
  if (argc == 3) {
    port = atoi(argv[1]);
    root = argv[2];
  }

  struct sigaction sa = {0};
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  
  char* ftp_root = malloc(PATH_MAX);
  if (!ftp_root) {
    perror("malloc ftp_root failed");
    return 1;
  }

  struct passwd *pw = getpwuid(getuid());
  if (!pw) {
    perror("Error getting FTP root.");
    free(ftp_root);
    return 2;
  }

  snprintf(ftp_root, PATH_MAX, "%s/Semestralka/%s", pw->pw_dir, root);

  //creates TCP IPv4 socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("error creating main socket");
    free(ftp_root);
    return 3;
  }
  
  // can reuse port
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("binding socket to the port failed");
    close(server_fd);
    free(ftp_root);  
    return 4;
  }
  
  // sets main socket as passive
  if (listen(server_fd, 10) < 0) {
    perror("error setting main socket into passive mode");
    close(server_fd);
    free(ftp_root);
    return 5;
  }

  printf("FTP server listening on port 2121...\n");
  printf("FTP root is: %s\n", ftp_root);
  
  user_table_t* user_table = malloc(sizeof(user_table_t));
  init_user_table(user_table);
  //for testing only
  add_user("test", "123", user_table);

  pthread_t console_thread;
  console_arg_t* c_arg = malloc(sizeof(console_arg_t));
  c_arg->user_table = user_table;
  c_arg->shutdown_requested = &shutdown_requested;

  if (pthread_create(&console_thread, NULL, console_manager_thread, c_arg) < 0) {
    perror("Error creating console manager thread.\n");
    goto cleanup;
  }
  pthread_detach(console_thread);
  
  active_client_registry_t* client_registry = malloc(sizeof(active_client_registry_t));
  init_client_registry(client_registry);
  
  pthread_t scanner_thread;
  if (pthread_create(&scanner_thread, NULL, scan_clients, client_registry) < 0) {
    perror("Error creating scanner thread.\n");
    goto cleanup;
  }

  while (!shutdown_requested) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      if (shutdown_requested) break;
      perror("Error accepting a client.\n");
      continue;
    }

    client_t* client = malloc(sizeof(client_t));
    if (!client) {
      perror("Malloc client failed.\n");
      close(client_fd);
      continue;
    }

    client->logged_in = 0;
    client->socket_fd = client_fd;
    client->ctrl_addr = client_addr;
    client->data_fd = -1;
    client->pasv_fd = -1;
    client->file_fd = -1;
    client->abort_requested = 0;

    client_thread_arg_t* client_thread_arg = malloc(sizeof(client_thread_arg_t));
    client_thread_arg->client_registry = client_registry;
    client_thread_arg->user_table = user_table;
    client_thread_arg->client = client;
    client_thread_arg->ftp_root = ftp_root;

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, client_thread_arg) < 0) {
      perror("Client pthread_create failed.\n");
      close(client_fd);
      free(client);
      continue;
    }
    
    pthread_detach(tid);
  }

  cleanup:
    if (client_registry) {
      clear_client_registry(client_registry);
      free(client_registry);
    }    
    clear_user_table(user_table);
    free(user_table);
    close(server_fd);
    free(ftp_root);

  return 0;
}
