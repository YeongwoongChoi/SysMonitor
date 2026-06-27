#include <dlfcn.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "common.h"
#define DEFAULT_PORT 8080

typedef struct {
	char name[16];
	void *module_handler;
} LoadedModule;

typedef struct {
	struct sockaddr_in client_addr;
	socklen_t addr_len;
	char request_type[16];
} RequestData;

void cleanup_modules();
void terminate(int sig);
void *get_module_handler(const char *request_type);

void initialize_server(struct sockaddr_in *server_addr, int port);
void process_request(struct sockaddr_in *client_addr, socklen_t address_length);

// Get response from module specified by 'request_type'
char *get_response(int sock, const char *request_type);

const char *get_time();
