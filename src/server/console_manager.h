#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#include "user_table.h"
#include <signal.h>
#define STR(s) #s
#define XSTR(s) STR(s)

typedef struct {
  user_table_t* user_table;
  volatile sig_atomic_t* shutdown_requested;
} console_arg_t;

void* console_manager_thread(void* arg);

#endif
