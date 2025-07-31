#include "server.h"
#include "resource.h"
#include "module.h"

static CachedModule loaded_modules[TOTAL_MODULES];
static int module_count = 0;
static int socket_descriptor;

void print_log(char **buf, int *remained, const char *log_type, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char message[BUF_SIZE];
    vsnprintf(message, sizeof(message), format, args);

    if (buf && remained && *buf && *remained > 0) {
		int written = snprintf(*buf, *remained, "%s%s%s\n", log_type, message, LOG_DEFAULT);
		if (written > 0) {
			if (written < *remained) {
				*buf += written;
				*remained -= written;
			}
			else {
				*buf += *remained;
				*remained = 0;
			}
		}
	}
    else
        printf("%s%s%s\n", log_type, message, LOG_DEFAULT);

    va_end(args);
}

const char *get_time() {
	static char datetime[30];
	time_t raw = time(NULL);
	struct tm *info = localtime(&raw);
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S %Z", info);

	return datetime;
}

void cleanup_modules() {
	print_log(NULL, NULL, LOG_INFO, "Cleaning up loaded modules.");
	for (int i = 0; i < module_count; ++i) {
		if (loaded_modules[i].module_handler) {
			dlclose(loaded_modules[i].module_handler);
			print_log(NULL, NULL, LOG_INFO, "Module %s closed.", loaded_modules[i].name);
		}
	}

	if (socket_descriptor > 0)
		close(socket_descriptor);
}

void terminate(int sig) {
	print_log(NULL, NULL, LOG_INFO, "SIGINT received. Server closed.");
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
		print_log(NULL, NULL, LOG_ERROR, "Cannot open module %s: %s", module_path, dlerror());
		return NULL;
	}
	print_log(NULL, NULL, LOG_SUCCESS, "Module %s loaded.", module_path);

	if (module_count < TOTAL_MODULES) {
		strcpy(loaded_modules[module_count].name, request_type);
		loaded_modules[module_count].module_handler = handler;
		module_count++;
	}
	return handler;
}

char *handle_request(int sock, const char *request_type) {
	char *error_message = NULL;
	void *handler = get_module_handler(request_type);
	if (!handler)
		return NULL;
	
	dlerror();		// flush previous error
	Module* (*get_module)() = dlsym(handler, "get_module");

	if (!get_module && (error_message = dlerror())) {
		print_log(NULL, NULL, LOG_ERROR, "Cannot find symbol 'get_module': %s", error_message);
		return NULL;
	}

	Module *mod = get_module();
	if (mod->initialize_module)
		mod->initialize_module(print_log);

	char *result = mod->respond_data();
	return result;
}

int main() {
	struct sockaddr_in client_addr, server_addr;
	socklen_t address_length = sizeof(server_addr);
	char buf[BUF_SIZE], client_ip[INET_ADDRSTRLEN];

	atexit(cleanup_modules);
	signal(SIGINT, terminate);

	if ((socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		print_log(NULL, NULL, LOG_ERROR, "Error occurred while socket() executed.");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_descriptor, (struct sockaddr *)(&server_addr), sizeof(server_addr)) < 0) {
		print_log(NULL, NULL, LOG_ERROR, "Error occurred while binding socket.");
		close(socket_descriptor);
		exit(1);
	}
	print_log(NULL, NULL, LOG_SUCCESS, "Binding was successful.");
	print_log(NULL, NULL, LOG_INFO, "Monitoring agent started at %s on port %d.", get_time(), PORT_NUM);

	int received;
	char *request_type, *response;

	while (1) {
		received = recvfrom(socket_descriptor, buf, BUF_SIZE, 0, 
			(struct sockaddr *)(&client_addr), &address_length);

		if (received < 0) {
			print_log(NULL, NULL, LOG_ERROR, "Error occurred while receiving data from client.");
			exit(1);
		}
		buf[received] = '\0';
		buf[strcspn(buf, "\r\n")] = '\0';
		request_type = strdup(buf);
		
		char *p = buf;
		received = BUF_SIZE;
		memset(buf, 0, BUF_SIZE);
		
		if (!strcmp(request_type, "cpu") || !strcmp(request_type, "mem") || !strcmp(request_type, "disk")) {
			response = handle_request(socket_descriptor, request_type);
			if (response) {
				print_log(&p, &received, LOG_INFO, response);
				inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
				print_log(NULL, NULL, LOG_INFO, "Sent %u bytes to client(%s) for request '%s'.", 
					BUF_SIZE - received, client_ip, request_type);
			}
			else
				print_log(NULL, NULL, LOG_ERROR, "Error occurred while getting response from module.");
		}
        else
            print_log(&p, &received, LOG_ERROR, "[SysMonitor] Invalid request. %s", 
                "Request should be one of these: [cpu|mem|disk]", request_type);
		sendto(socket_descriptor, buf, BUF_SIZE - received, 0, 
				(struct sockaddr *)(&client_addr), address_length);
		free(request_type);
	}
	return 0;
}
