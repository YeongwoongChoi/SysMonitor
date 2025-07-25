#include "server.h"
#include "resource.h"

void print_log(char *buf, const char *log_type, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char message[500];
    vsnprintf(message, sizeof(message), format, args);

    if (buf)
        snprintf(buf, BUF_SIZE, "%s%s%s\n", log_type, message, LOG_DEFAULT);
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

int main() {
	int socket_descriptor;
	struct sockaddr_in client_addr, server_addr;

	socklen_t address_length = sizeof(server_addr);
	char buf[BUF_SIZE];

	if ((socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		print_log(NULL, LOG_ERROR, "Error occurred while socket() executed.");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_descriptor, (struct sockaddr *)(&server_addr), sizeof(server_addr)) < 0) {
		print_log(NULL, LOG_ERROR, "Error occurred while binding socket.");
		exit(1);
	}
	print_log(NULL, LOG_SUCCESS, "Binding was successful.");
	print_log(NULL, LOG_INFO, "Monitoring agent started at %s on port %d.", get_time(), PORT_NUM);

	int received, sent;

	while (1) {
		received = recvfrom(socket_descriptor, buf, BUF_SIZE, 0, 
			(struct sockaddr *)(&client_addr), &address_length);

		if (received < 0) {
			print_log(NULL, LOG_ERROR, "Error occurred while receiving data from client.");
			exit(1);
		}
		buf[received] = '\0';
		buf[strcspn(buf, "\r\n")] = '\0';

		if (!strcmp(buf, "cpu"))
			print_log(buf, LOG_INFO, "[SysMonitor] CPU Usage: %.2f%%", get_cpu_usage());
		else if (!strcmp(buf, "mem"))
        	print_log(buf, LOG_INFO, "[SysMonitor] Mem Usage: %.2f%%", get_mem_usage());
		else if (!strcmp(buf, "disk"))
            print_log(buf, LOG_INFO, "[SysMonitor] Disk Usage: %.2f%%", get_disk_usage("/"));
        else
            print_log(buf, LOG_ERROR, "[SysMonitor] Invalid request. %s", 
                "Request should be one of these: [cpu|mem|disk]");

		int total_sent = 0;
		const int length = strlen(buf);

		while (total_sent < length) {
			sent = sendto(socket_descriptor, buf + total_sent, length - total_sent, 0, 
				(struct sockaddr *)(&client_addr), address_length);

			if (sent == -1) {
				print_log(NULL, LOG_ERROR, "Error occurred while sending data to client.");
				exit(1);
			}
			total_sent += sent;
		}
	}
	close(socket_descriptor);
	return 0;
}
