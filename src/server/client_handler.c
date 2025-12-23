#include "client_handler.h"
#include "active_client_registry.h"
#include "client.h"
#include "ftp_utils.h"
#include "user_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

user_t* handle_connection(client_t *client, user_table_t *user_table, active_client_registry_t* registry) {
  char buffer[512];
  
  send_response(client->socket_fd, "220", "FTP Server Ready");
  send_response(client->socket_fd, "331", "Please enter your username:");
  
  user_t* user;
  while (1) {
    if (!receive_line(client, buffer, sizeof(buffer))) {
      client_registry_remove(registry, client);
      return NULL;
    }

    user = find_user(buffer, user_table);
    if (user == NULL) {
      printf("not found");
      fflush(stdout);
      send_response_fmt(client->socket_fd, "530", "User %s doesn't exit, try again", buffer);
      continue;
    }
    break;
  }
  
  send_response_fmt(client->socket_fd, "331", "Hello %s, enter your password:", user->username);

  while (1) {
    if (!receive_line(client, buffer, sizeof(buffer))) {
      client_registry_remove(registry, client);
      return NULL;
    }

    if (strcmp(user->password, buffer) != 0) {
      send_response(client->socket_fd, "530", "Wrong password, try again");
      continue;
    }
    break;
  }
  return user;
}

void* handle_client(void* arg) {
  client_thread_arg_t* client_arg = (client_thread_arg_t*)arg;
  client_registry_add(client_arg->client_registry, client_arg->client);
  user_t* user = handle_connection(client_arg->client, client_arg->user_table, client_arg->client_registry);
  
  if (user == NULL) {
    goto out;
  }

  client_arg->client->logged_in = 1;
  
  snprintf(client_arg->client->cwd, sizeof(client_arg->client->cwd), "%s/%s", client_arg->ftp_root, user->username);
  client_arg->client->cwd[sizeof(client_arg->client->cwd)-1] = '\0';
  
  strcpy(client_arg->client->home_dir, client_arg->client->cwd);
  
  struct stat st = {0};
  if (stat(client_arg->client->home_dir, &st) == -1) {
    if (mkdir(client_arg->client->home_dir, 0755) == -1) {
      perror("mkdir failed");
      send_response(client_arg->client->socket_fd, "550", "Failed to create home directory");
      goto out;
    }
  }

  send_response(client_arg->client->socket_fd, "230", "User logged in, proceed");

  char buffer[512] = {0};
  while (!*(client_arg->shutdown_requested)) {
    if (!receive_line(client_arg->client, buffer, sizeof(buffer))) 
      break;
    
    
    int user_quit = process_command(client_arg->client, buffer);
    if (user_quit)
      break;
  }
  out:
    client_registry_remove(client_arg->client_registry, client_arg->client);
    free(client_arg);
    return NULL;
}
