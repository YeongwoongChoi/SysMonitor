#include "server.h"
#include "resource.h"
#include "module.h"

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

char *handle_request(int sock, const char *request_type) {
	char buf[BUF_SIZE];
	snprintf(buf, BUF_SIZE, "bin/modules/%s.so", request_type);
	char *error_message = NULL;

	void *handler = dlopen(buf, RTLD_NOW);
	if (!handler) {
		error_message = dlerror();
		print_log(NULL, NULL, LOG_ERROR, "Cannot open module %s\n %s\n", buf, error_message);
		return NULL;
	}
	dlerror();		// flush previous error
	Module* (*get_module)() = dlsym(handler, "get_module");

	if (!get_module && (error_message = dlerror())) {
		print_log(NULL, NULL, LOG_ERROR, "Cannot find symbol 'get_module': %s", error_message);
		dlclose(handler);
		return NULL;
	}

	Module *mod = get_module();
	if (mod->initialize_module)
		mod->initialize_module(print_log);

	char *result = mod->respond_data();
	return result;
}

int main() {
	int socket_descriptor;
	struct sockaddr_in client_addr, server_addr;

	socklen_t address_length = sizeof(server_addr);
	char buf[BUF_SIZE], client_ip[INET_ADDRSTRLEN];

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
