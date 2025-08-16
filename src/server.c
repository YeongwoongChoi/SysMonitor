#include "server.h"
#include "resource.h"
#include "module.h"

static LoadedModule loaded_modules[TOTAL_MODULES];
static int module_count = 0;
static int socket_descriptor;

const char *get_time() {
	static char datetime[30];
	time_t raw = time(NULL);
	struct tm *info = localtime(&raw);
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S %Z", info);

	return datetime;
}

void cleanup_modules() {
	puts("Cleaning up loaded modules.");
	for (int i = 0; i < module_count; ++i) {
		if (loaded_modules[i].module_handler) {
			dlclose(loaded_modules[i].module_handler);
			printf("Module bin/modules/%s.so closed.\n", loaded_modules[i].name);
		}
	}

	if (socket_descriptor > 0) {
		close(socket_descriptor);
		puts("Socket closed.");
	}
}

void terminate(int sig) {
	printf("SIGINT received. Monitoring agent stopped at %s.\n", get_time());
	exit(0);
}

void *get_module_handler(const char *request_type) {
	for (int i = 0; i < module_count; ++i) {
		if (!strcmp(loaded_modules[i].name, request_type))
			return loaded_modules[i].module_handler;
	}
	char module_path[BUF_SIZE];
	snprintf(module_path, BUF_SIZE, "bin/modules/%s.so", request_type);

	void *handler = dlopen(module_path, RTLD_NOW);
	if (!handler) {
		fprintf(stderr, "Cannot open module %s: %s\n", module_path, dlerror());
		return NULL;
	}
	printf("Module %s loaded.\n", module_path);

	if (module_count < TOTAL_MODULES) {
		strcpy(loaded_modules[module_count].name, request_type);
		loaded_modules[module_count].module_handler = handler;
		module_count++;
	}
	return handler;
}

void initialize_server(struct sockaddr_in *server_addr) {
	atexit(cleanup_modules);
	signal(SIGINT, terminate);

	if ((socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Error occurred while socket() executed.\n");
		exit(1);
	}

	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(PORT_NUM);
	server_addr->sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_descriptor, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
		fprintf(stderr, "Error occurred while binding socket.\n");
		exit(1);
	}
	puts("Binding was successful.");
	printf("Monitoring agent started at %s on port %d.\n", get_time(), PORT_NUM);
}

void process_request(struct sockaddr_in *client_addr, socklen_t address_length) {
	char buf[BUF_SIZE], client_ip[INET_ADDRSTRLEN];

	int received, sent = 0, remained = BUF_SIZE - 1, set_exit = 0;
	char *request_type, *response;

	received = recvfrom(socket_descriptor, buf, BUF_SIZE, 0, 
			(struct sockaddr *)client_addr, &address_length);

	if (received < 0) {
		fprintf(stderr, "Error occurred while receiving data from client.\n");
		exit(1);
	}
	buf[received] = '\0';
	buf[strcspn(buf, "\r\n")] = '\0';
	request_type = strdup(buf);
	
	if (!strcmp(request_type, "cpu") || !strcmp(request_type, "mem") || !strcmp(request_type, "disk")) {
		response = get_response(socket_descriptor, request_type);
		if (response) {
			sent += snprintf(buf + sent, remained - sent, "%s", response);
			free(response);
			inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
			printf("Sent %u bytes to client(%s) for request '%s'.\n", 
				sent, client_ip, request_type);
		}
		else
			fprintf(stderr, "Error occurred while getting response from module.\n");
	}
	else if (!strcmp(request_type, "exit")) {
		sent += snprintf(buf + sent, remained - sent, 
			"[SysMonitor] Server closed - connection will be lost.\n");
		sent += snprintf(buf + sent, remained - sent, 
			"[SysMonitor] You can immediately terminate client program.\n");
		set_exit = 1;
	}
	else
		sent += snprintf(buf + sent, remained - sent, 
			"[SysMonitor] Invalid request - \"%s\"\n%s\n", request_type, 
			"Request should be one of these: [cpu|mem|disk|exit]");
	sendto(socket_descriptor, buf, sent, 0, (struct sockaddr *)client_addr, address_length);
	free(request_type);
	if (set_exit)
		exit(0);
}

char *get_response(int sock, const char *request_type) {
	char *error_message = NULL;
	void *handler = get_module_handler(request_type);
	if (!handler)
		return NULL;
	
	dlerror();		// flush previous error
	Module* (*get_module)() = dlsym(handler, "get_module");

	if (!get_module && (error_message = dlerror())) {
		fprintf(stderr, "Cannot find symbol 'get_module': %s\n", error_message);
		return NULL;
	}

	Module *mod = get_module();
	char *result = mod->respond_data();
	return result;
}

int main() {
	struct sockaddr_in client_addr, server_addr;
	socklen_t address_length = sizeof(server_addr);
	initialize_server(&server_addr);

	while (1)
		process_request(&client_addr, address_length);
	return 0;
}
