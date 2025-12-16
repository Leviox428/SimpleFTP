#ifndef CONSOLE_MANAGER_H
#define CONSOLE_MANAGER_H

#define STR(s) #s
#define XSTR(s) STR(s)

void* console_manager_thread(void* arg);

#endif
