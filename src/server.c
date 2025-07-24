#include "server.h"

void print_log(const char *log_type, const char *format, ...) {
	va_list args;
	va_start(args, format);

	printf("%s", log_type);
	vprintf(format, args);
	printf("%s\n", LOG_DEFAULT);

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
		print_log(LOG_ERROR, "Error occurred while socket() executed.");
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_descriptor, (struct sockaddr *)(&server_addr), sizeof(server_addr)) < 0) {
		print_log(LOG_ERROR, "Error occurred while binding socket.");
		exit(1);
	}
	print_log(LOG_SUCCESS, "Binding was successful.");
	print_log(LOG_INFO, "Monitoring agent started at %s on port %d.", get_time(), PORT_NUM);

	int received, sent;

	while (1) {
		received = recvfrom(socket_descriptor, buf, BUF_SIZE, 0, 
			(struct sockaddr *)(&client_addr), &address_length);

		if (received < 0) {
			print_log(LOG_ERROR, "Error occurred while receiving data from client.");
			exit(1);
		}
		buf[received] = '\0';

		print_log(LOG_DEFAULT, "Message from client was %s", buf);
		
		snprintf(buf, BUF_SIZE, "[Response from server] Current time: %s\n", get_time());
		int total_sent = 0;
		const int length = strlen(buf);

		while (total_sent < length) {
			sent = sendto(socket_descriptor, buf + total_sent, length - total_sent, 0, 
				(struct sockaddr *)(&client_addr), address_length);

			if (sent == -1) {
				print_log(LOG_ERROR, "Error occurred while sending data to client.");
				exit(1);
			}
			total_sent += sent;
		}
	}
	close(socket_descriptor);
	return 0;
}
