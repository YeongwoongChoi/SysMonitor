#include "module.h"
#include "resource.h"

static log_function g_logger = NULL;

void initialize_disk_module(log_function logger) {
    g_logger = logger;
}

char *respond_disk_usage() {
    if (!g_logger) {
        g_logger(NULL, NULL, LOG_ERROR, "Disk module not initialized.");
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

    int count;
	DiskInfo *infos = read_disk_info(&count);
	if (!infos)
		return NULL;

	g_logger(&p, &remained, LOG_INFO,
		"%-85s\
		\n=====================================================================================",
		"[SysMonitor] Disk Usages");
	g_logger(&p, &remained, LOG_INFO,
		"|     %-10s |    %-16s |      %9s          |  %-15s |\
		\n=====================================================================================",
		"Device", "Mounted at", "Size", "Proportion (%)");
	
	struct statvfs stat;
	for (int i = 0; i < count; ++i) {
		if (!statvfs(infos[i].mountpoint, &stat)) {
			unsigned long long disk_total = (unsigned long long)stat.f_blocks * stat.f_frsize;
			unsigned long long disk_free = (unsigned long long)stat.f_bfree * stat.f_frsize;
			unsigned long long disk_used = disk_total - disk_free;
			
			char *tmp = strdup(convert_unit(disk_used));
			g_logger(&p, &remained, LOG_INFO, "| %-14s | %-20s| %10s / %10s | %14.2lf %% |",
				infos[i].device, infos[i].mountpoint, tmp, convert_unit(disk_total),
				get_proportion(disk_used, disk_total));
			free(tmp);
		}
	}
	g_logger(&p, &remained, LOG_INFO,
		"=====================================================================================");
	return buf;
}

Module *get_module() {
    static Module disk_module = {
        .name = "disk",
        .initialize_module = initialize_disk_module,
        .respond_data = respond_disk_usage
    };
    return &disk_module;
}
