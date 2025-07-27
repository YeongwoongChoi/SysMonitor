#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <stdarg.h>
#define BUF_SIZE (1 << 13)
#define PORT_NUM 8080

#define LOG_DEFAULT "\033[0m"
#define LOG_ERROR "\033[0;31m"
#define LOG_SUCCESS "\033[0;32m"
#define LOG_INFO "\033[0;94m"

// Write message on buf if both buf and remained are given, otherwise print on stdout
void print_log(char **buf, int *remained, const char *log_type, const char *format, ...);
void print_cpu_usage(char *buf, int *remained);
void print_mem_usage(char *buf, int *remained);
const char *convert_unit(unsigned long long byte);
const char *get_time();
