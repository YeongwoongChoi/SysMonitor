#include "module.h"
#include "resource.h"

char *respond_mem_usage() {
    char *buf = malloc(BUF_SIZE);
    if (!buf) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }
    memset(buf, 0, BUF_SIZE);
    int offset = 0, remained = BUF_SIZE - 1;

    MemStat stat = read_mem_stat();
	unsigned long long used = stat.mem_total - stat.mem_free - stat.buffers - stat.cached;

	offset += snprintf(buf + offset, remained - offset, 
		"\033[0;94m%-48s\n================================================\n", 
		"[SysMonitor] Memory Usages");
	offset += snprintf(buf + offset, remained - offset, 
		"|     %-8s |     %-8s | %-15s|\n================================================\n",
		"Name", "Size", "Proportion (%)");
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Total", convert_unit(stat.mem_total), 100.00);
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Used", convert_unit(used), get_proportion(used, stat.mem_total));
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Available", convert_unit(stat.mem_available), 
		get_proportion(stat.mem_available, stat.mem_total));
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Free", convert_unit(stat.mem_free), get_proportion(stat.mem_free, stat.mem_total));
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Shared", convert_unit(stat.shared), get_proportion(stat.shared, stat.mem_total));
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Buffers", convert_unit(stat.buffers), get_proportion(stat.buffers, stat.mem_total));
	offset += snprintf(buf + offset, remained - offset, "| %-10s   |   %10s | %12.2lf %% |\n", 
		"Cached", convert_unit(stat.cached), get_proportion(stat.cached, stat.mem_total));
	offset += snprintf(buf + offset, remained - offset, 
		"================================================\n\033[0m");
    return buf;
}

Module *get_module() {
    static Module mem_module = {
        .name = "mem",
        .respond_data = respond_mem_usage
    };
    return &mem_module;
}
