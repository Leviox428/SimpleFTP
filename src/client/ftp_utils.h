#ifndef FTP_UTILS_H
#define FTP_UTILS_H

#include <stddef.h>

int recieve_line(int fd, char *buffer, size_t buffer_size);
void send_line(int fd, char *line);
int stricmp(const char* a, const char *b);
int strincmp(const char *a, const char *b, size_t n);
#endif
