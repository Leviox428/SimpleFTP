#include "console_manager.h"
#include "user_table.h"
#include "ftp_utils.h"
#include <stdio.h>
#include <string.h>

void* console_manager_thread(void* arg) {
  user_table_t* user_table = (user_table_t*)arg;
  while (1) {
    char command_buffer[256];
    printf("> ");
    fflush(stdout);

    if (!fgets(command_buffer, sizeof(command_buffer), stdin)) break;
    command_buffer[strcspn(command_buffer, "\n")] = 0;
    
    size_t n = strlen("add");
    if (strincmp(command_buffer, "add", n) == 0) {

      char username[MAX_USERNAME + 1], password[MAX_PASSWORD + 1];

      if (sscanf(command_buffer + n, "%" XSTR(MAX_USERNAME) "s %" XSTR(MAX_PASSWORD) "s", username, password) == 2)  {

        int success = add_user(username, password, user_table);
        if (success) 
          printf("User %s added.\n", username);
        else 
          printf("User %s already exists.\n", username);
    
      } else {
          printf("Provide username and password please.\n");
      }
      fflush(stdout);
      continue;
    }
    
    n = strlen("remove");
    if (strincmp(command_buffer, "remove", n) == 0) {
      char username[MAX_USERNAME + 1];
      if (sscanf(command_buffer + n, "%" XSTR(MAX_USERNAME) "s", username) == 1) {
        int success = remove_user(username, user_table);
        if (success) 
          printf("User %s removed.\n", username);
        else 
          printf("User %s doesn't exists.\n", username);
      } else {
        printf("Password wasn't provided,\n");
      }
      fflush(stdout);
      continue;
    }

    if (stricmp(command_buffer, "quit") == 0) {
      printf("Quitting the server\n");
      fflush(stdout);
    }
  }
  return NULL;
}
