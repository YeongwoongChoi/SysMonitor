#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <stdarg.h>

#define BUF_SIZE (1 << 9)
#define PORT_NUM 8080

#define LOG_DEFAULT "\033[0m"
#define LOG_ERROR "\033[0;31m"
#define LOG_SUCCESS "\033[0;32m"
#define LOG_INFO "\033[0;34m"

void print_log(const char *log_type, const char *format, ...);
const char *get_time();
