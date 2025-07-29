#include "module.h"
#include "resource.h"

static log_function g_logger = NULL;

void initialize_cpu_module(log_function logger) {
    g_logger = logger;
}

char *respond_cpu_usage() {
    if (!g_logger) {
        g_logger(NULL, NULL, LOG_ERROR, "CPU module not initialized.");
        return NULL;
    }
    char *buf = malloc(BUF_SIZE);

    if (!buf) {
        g_logger(NULL, NULL, LOG_ERROR, "Failed to allocate memory.");
        return NULL;
    }
    memset(buf, 0, BUF_SIZE);
    char *p = buf;
    int remained = BUF_SIZE;

    const int core_count = get_core_count();
	CPUStat *prev_stats = read_cpu_stats(core_count);
	usleep(300000);
	CPUStat *curr_stats = read_cpu_stats(core_count);

	g_logger(&p, &remained, LOG_INFO, "%-31s\n===============================", 
		"[SysMonitor] CPU Core Usages");
	g_logger(&p, &remained, LOG_INFO, 
		"|    %-8s  |  %7s     |\n===============================", "Core #", "Load");
	
	g_logger(&p, &remained, LOG_INFO, "| %-10s   | %10.2lf %% |", 
		"All Cores", get_cpu_usage(prev_stats[0], curr_stats[0]));
	for (int i = 1; i <= core_count; ++i)
		g_logger(&p, &remained, LOG_INFO, "| Core %-7d | %10.2lf %% |", i,
			get_cpu_usage(prev_stats[i], curr_stats[i]));

	g_logger(&p, &remained, LOG_INFO, "===============================");
	free(prev_stats);
	free(curr_stats);
    return buf;
}

Module *get_module() {
    static Module cpu_module = {
        .name = "cpu",
        .initialize_module = initialize_cpu_module,
        .respond_data = respond_cpu_usage
    };
    return &cpu_module;
}
