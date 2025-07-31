#include <dlfcn.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include "common.h"
#define PORT_NUM 8080

// Caching module handles
typedef struct {
	char name[16];
	void *module_handler;
} CachedModule;

void cleanup_modules();
void terminate(int sig);
void *get_module_handler(const char *request_type);

void initialize_server(struct sockaddr_in *server_addr);
void process_request(struct sockaddr_in *client_addr, socklen_t address_length);

// Get response from module specified by 'request_type'
char *get_response(int sock, const char *request_type);

// Write message on buf if both buf and remained are given, otherwise print on stdout
void print_log(char **buf, int *remained, const char *log_type, const char *format, ...);
const char *get_time();
