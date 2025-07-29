#include "module.h"
#include "resource.h"

static log_function g_logger = NULL;

void initialize_mem_module(log_function logger) {
    g_logger = logger;
}

char *respond_mem_usage() {
    if (!g_logger) {
        g_logger(NULL, NULL, LOG_ERROR, "Memory module not initialized.");
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

    MemStat stat = read_mem_stat();
	unsigned long long used = stat.mem_total - stat.mem_free - stat.buffers - stat.cached;

	g_logger(&p, &remained, LOG_INFO, 
		"%-48s\n================================================", "[SysMonitor] Memory Usages");
	g_logger(&p, &remained, LOG_INFO, 
		"|     %-8s |     %-8s | %-15s|\n================================================",
		"Name", "Size", "Proportion (%)");
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Total", convert_unit(stat.mem_total), 100.00);
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Used", convert_unit(used), get_proportion(used, stat.mem_total));
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Available", convert_unit(stat.mem_available), 
		get_proportion(stat.mem_available, stat.mem_total));
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Free", convert_unit(stat.mem_free), get_proportion(stat.mem_free, stat.mem_total));
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Shared", convert_unit(stat.shared), get_proportion(stat.shared, stat.mem_total));
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Buffers", convert_unit(stat.buffers), get_proportion(stat.buffers, stat.mem_total));
	g_logger(&p, &remained, LOG_INFO, "| %-10s   |   %10s | %12.2lf %% |", 
		"Cached", convert_unit(stat.cached), get_proportion(stat.cached, stat.mem_total));
	g_logger(&p, &remained, LOG_INFO, "================================================");
    return buf;
}

Module *get_module() {
    static Module mem_module = {
        .name = "mem",
        .initialize_module = initialize_mem_module,
        .respond_data = respond_mem_usage
    };
    return &mem_module;
}
