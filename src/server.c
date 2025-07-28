#include "server.h"
#include "resource.h"

void print_log(char **buf, int *remained, const char *log_type, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char message[500];
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

void print_cpu_usage(char *buf, int *remained) {
	const int core_count = get_core_count();
	CPUStat *prev_stats = read_cpu_stats(core_count);
	usleep(300000);
	CPUStat *curr_stats = read_cpu_stats(core_count);

	print_log(&buf, remained, LOG_INFO, "%-31s\n===============================", 
		"[SysMonitor] CPU Core Usages");
	print_log(&buf, remained, LOG_INFO, 
		"|    %-8s  |  %7s     |\n===============================", "Core #", "Load");
	
	print_log(&buf, remained, LOG_INFO, "| %-10s   | %10.2lf %% |", 
		"All Cores", get_cpu_usage(prev_stats[0], curr_stats[0]));
	for (int i = 1; i <= core_count; ++i)
		print_log(&buf, remained, LOG_INFO, "| Core %-7d | %10.2lf %% |", i,
			get_cpu_usage(prev_stats[i], curr_stats[i]));

	print_log(&buf, remained, LOG_INFO, "===============================");
	free(prev_stats);
	free(curr_stats);
}

void print_mem_usage(char *buf, int *remained) {
	MemStat stat = read_mem_stat();
	unsigned long long used = stat.mem_total - stat.mem_free - stat.buffers - stat.cached;

	print_log(&buf, remained, LOG_INFO, 
		"%-48s\n================================================", "[SysMonitor] Memory Usages");
	print_log(&buf, remained, LOG_INFO, 
		"|     %-8s |     %-8s | %-15s|\n================================================",
		"Name", "Size", "Proportion (%)");
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Total", convert_unit(stat.mem_total), 100.00);
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Used", convert_unit(used), get_proportion(used, stat.mem_total));
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Available", convert_unit(stat.mem_available), 
		get_proportion(stat.mem_available, stat.mem_total));
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Free", convert_unit(stat.mem_free), get_proportion(stat.mem_free, stat.mem_total));
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Shared", convert_unit(stat.shared), get_proportion(stat.shared, stat.mem_total));
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Buffers", convert_unit(stat.buffers), get_proportion(stat.buffers, stat.mem_total));
	print_log(&buf, remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Cached", convert_unit(stat.cached), get_proportion(stat.cached, stat.mem_total));
	print_log(&buf, remained, LOG_INFO, "================================================");
}	

void print_disk_usage(char *buf, int *remained) {
	int count;
	DiskInfo *infos = read_disk_info(&count);
	if (!infos)
		return;

	print_log(&buf, remained, LOG_INFO,
		"%-82s\
		\n==================================================================================",
		"[SysMonitor] Disk Usages");
	print_log(&buf, remained, LOG_INFO,
		"|     %-10s |    %-13s |           %-10s      | %-15s|\
		\n==================================================================================",
		"Device", "Mounted at", "Size", "Proportion (%)");
	
	struct statvfs stat;
	for (int i = 0; i < count; ++i) {
		if (!statvfs(infos[i].mountpoint, &stat)) {
			unsigned long long disk_total = (unsigned long long)stat.f_blocks * stat.f_frsize;
			unsigned long long disk_free = (unsigned long long)stat.f_bfree * stat.f_frsize;
			unsigned long long disk_used = disk_total - disk_free;
			
			char *tmp = strdup(convert_unit(disk_used));
			print_log(&buf, remained, LOG_INFO, "| %-14s | %-17s| %11s / %11s | %12.2lf %% |",
				infos[i].device, infos[i].mountpoint, tmp, convert_unit(disk_total),
				get_proportion(disk_used, disk_total));
			free(tmp);
		}
	}
	print_log(&buf, remained, LOG_INFO,
		"==================================================================================");
}

const char *convert_unit(unsigned long long byte) {
	static char buf[32];
	if (byte >= (1 << 30))
		snprintf(buf, sizeof(buf), "%.2f GB", byte / (double)(1 << 30));
	else if (byte >= (1 << 20))
		snprintf(buf, sizeof(buf), "%.2f MB", byte / (double)(1 << 20));
	else if (byte >= (1 << 10))
		snprintf(buf, sizeof(buf), "%.2f KB", byte / (double)(1 << 10));
	else
		snprintf(buf, sizeof(buf), "%llu", byte);
	return buf;
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

	int received, sent;
	char *request_type;

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
		memset(buf, 0, BUF_SIZE);
		char *p = buf;
		int remained = BUF_SIZE;

		if (!strcmp(request_type, "cpu"))
			print_cpu_usage(p, &remained);
		else if (!strcmp(request_type, "mem"))
        	print_mem_usage(p, &remained);
		else if (!strcmp(request_type, "disk"))
            print_disk_usage(p, &remained);
        else
            print_log(&p, &remained, LOG_ERROR, "[SysMonitor] Invalid request. %s", 
                "Request should be one of these: [cpu|mem|disk]");

		int total_sent = 0;
		const int length = BUF_SIZE - remained;
		while (total_sent < length) {
			sent = sendto(socket_descriptor, buf + total_sent, length - total_sent, 0, 
				(struct sockaddr *)(&client_addr), address_length);

			if (sent == -1) {
				print_log(NULL, NULL, LOG_ERROR, "Error occurred while sending data to client.");
				exit(1);
			}
			total_sent += sent;
		}
		inet_ntop(AF_INET, &(client_addr.sin_addr), buf, INET_ADDRSTRLEN);
		print_log(NULL, NULL, LOG_INFO, "Sent %u bytes to client(%s).", BUF_SIZE - remained, buf);
		free(request_type);
	}
	close(socket_descriptor);
	return 0;
}
