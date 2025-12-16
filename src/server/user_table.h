#ifndef USER_TABLE_H
#define USER_TABLE_H

#include <pthread.h>

#define USER_TABLE_CAPACITY 1024
#define MAX_USERNAME 31
#define MAX_PASSWORD 63

typedef struct user {
  char username[MAX_USERNAME + 1];
  char password[MAX_PASSWORD + 1];
  struct user* next;
  struct user** pprev;
} user_t;

typedef struct {
  pthread_mutex_t mutex;
  user_t* users[USER_TABLE_CAPACITY];
  size_t user_count;
} user_table_t;

void init_user_table(user_table_t* self);
unsigned int hash_username(const char* username);
int add_user(const char* username, const char* password, user_table_t* self);
int remove_user(const char* username, user_table_t* self);
user_t* find_user(const char* username, user_table_t* self);
void clear_user_table(user_table_t* self);

#endif 
