#include "module.h"
#include "resource.h"

char *respond_cpu_usage(const char* request_type) {
	char *buf = malloc(BUF_SIZE);
	if (!buf) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return NULL;
	}
	memset(buf, 0, BUF_SIZE);
	int offset = 0, remained = BUF_SIZE - 1;

	const int core_count = get_core_count();
	CPUStat *prev_stats = read_cpu_stats(core_count);
	usleep(300000);
	CPUStat *curr_stats = read_cpu_stats(core_count);

	if (!prev_stats || !curr_stats) {
		free(prev_stats);
		free(curr_stats);
		char* msg = malloc(64);
		if (msg)
			strcpy(msg, "Error: Failed to read CPU stats.");
		return msg;
	}

	if (strstr(request_type, ":json")) {
		offset += snprintf(buf + offset, remained - offset, "{\"type\": \"cpu\", \"total\": %.2lf, \"cores\": [", get_cpu_usage(prev_stats[0], curr_stats[0]));
		
		for (int i = 1; i <= core_count; ++i) {
			offset += snprintf(buf + offset, remained - offset, "%.2lf%s", get_cpu_usage(prev_stats[i], curr_stats[i]), (i == core_count ? "": ", "));
		
		}
		if (offset < BUF_SIZE)
			snprintf(buf + offset, remained - offset, "]}");
	}
	else {
		offset += snprintf(buf + offset, remained - offset, 
				"\033[0;94m%-31s\n===============================\n", "[SysMonitor] CPU Core Usages");
		offset += snprintf(buf + offset, remained - offset, 
				"|    %-8s  |  %7s     |\n===============================\n", "Core #", "Load");
		offset += snprintf(buf + offset, remained - offset, "| %-10s   | %10.2lf %% |\n", "All Cores", 
				get_cpu_usage(prev_stats[0], curr_stats[0]));

		for (int i = 1; i <= core_count; ++i)
			offset += snprintf(buf + offset, remained - offset, "| Core %-7d | %10.2lf %% |\n", i,
					get_cpu_usage(prev_stats[i], curr_stats[i]));

		offset += snprintf(buf + offset, remained - offset, 
				"===============================\n\033[0m");
	}

	free(prev_stats);
	free(curr_stats);
	return buf;
}

Module *get_module() {
	static Module cpu_module = {
		.name = "cpu",
		.respond_data = respond_cpu_usage
	};
	return &cpu_module;
}
