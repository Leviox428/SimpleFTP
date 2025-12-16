#include "user_table.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void init_user_table(user_table_t *self) {
  pthread_mutex_init(&self->mutex, NULL);
  self->user_count = 0;
}

unsigned int hash_username(const char *username) {
  unsigned int hash = 5381;
  while (*username) {
    hash = ((hash << 5) + hash) + (unsigned char)(*username++);
  }
  return hash % USER_TABLE_CAPACITY;
}

int add_user(const char *username, const char *password, user_table_t* self) {
  if (self->user_count == USER_TABLE_CAPACITY) {
    printf("Cannt add user, the user table is full\n.");
    fflush(stdout);
    return 0;
  }

  if (find_user(username, self) != NULL) {
    return 0;
  }
  unsigned int idx = hash_username(username);
  user_t* user = malloc(sizeof(user_t));
  strcpy(user->username, username);
  strcpy(user->password, password);
  pthread_mutex_lock(&self->mutex);

  user->next = self->users[idx];
  user->pprev = &self->users[idx];
  if (user->next) {
    user->next->pprev = &user->next;
  }
  self->users[idx] = user;
  self->user_count++;
  pthread_mutex_unlock(&self->mutex);
  return 1;
}

user_t* find_user(const char* username, user_table_t* self) {
  unsigned int idx = hash_username(username);
  pthread_mutex_lock(&self->mutex);
  user_t* u = self->users[idx];
  while (u) {
    if (strcmp(u->username, username) == 0) {
      pthread_mutex_unlock(&self->mutex);
      return u;
    }
    u = u->next;
  }
  pthread_mutex_unlock(&self->mutex);
  return NULL;
}

int remove_user(const char *username, user_table_t *self) {
  unsigned int idx = hash_username(username);
  pthread_mutex_lock(&self->mutex);
  
  user_t *u = self->users[idx];
  while (u) {
    if (strcmp(u->username, username) == 0) {
      *u->pprev = u->next;
      if (u->next) {
        u->next->pprev = u->pprev;
      }
      self->user_count--;
      pthread_mutex_unlock(&self->mutex);
      free(u);
      return 1;
    }
    u = u->next;
  }
  
  pthread_mutex_unlock(&self->mutex);
  return 0;
}

void clear_user_table(user_table_t *self) {
  pthread_mutex_lock(&self->mutex);
  for (int i = 0; i < USER_TABLE_CAPACITY; i++) {
    user_t* u = self->users[i];
  
    while (u) {
      user_t* next = u->next;
      free(u);
      u = next;
    }
    self->users[i] = NULL;
  }
  pthread_mutex_unlock(&self->mutex);
  pthread_mutex_destroy(&self->mutex);
}
