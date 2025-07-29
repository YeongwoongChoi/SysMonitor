#include <dlfcn.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <stdarg.h>
#include "common.h"
#define PORT_NUM 8080

// Respond to the client specified by 'client_addr'
void handle_request(int sock, const char *request_type, struct sockaddr_in *client_addr, 
	socklen_t address_length);
    
// Write message on buf if both buf and remained are given, otherwise print on stdout
void print_log(char **buf, int *remained, const char *log_type, const char *format, ...);
const char *get_time();
