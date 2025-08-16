#include "module.h"
#include "resource.h"

char *respond_cpu_usage() {
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
