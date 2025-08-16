#include "module.h"
#include "resource.h"

char *respond_disk_usage() {
    char *buf = malloc(BUF_SIZE);
	if (!buf) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }
    memset(buf, 0, BUF_SIZE);
    int offset = 0, remained = BUF_SIZE - 1;

    int count;
	DiskInfo *infos = read_disk_info(&count);
	if (!infos)
		return NULL;

	offset += snprintf(buf + offset, remained - offset,
		"\033[0;94m%-85s\
		\n=====================================================================================\n",
		"[SysMonitor] Disk Usages");
	offset += snprintf(buf + offset, remained - offset,
		"|     %-10s |    %-16s |      %9s          |  %-15s |\
		\n=====================================================================================\n",
		"Device", "Mounted at", "Size", "Proportion (%)");
	
	struct statvfs stat;
	for (int i = 0; i < count; ++i) {
		if (!statvfs(infos[i].mountpoint, &stat)) {
			unsigned long long disk_total = (unsigned long long)stat.f_blocks * stat.f_frsize;
			unsigned long long disk_free = (unsigned long long)stat.f_bfree * stat.f_frsize;
			unsigned long long disk_used = disk_total - disk_free;
			
			char *tmp = strdup(convert_unit(disk_used));
			offset += snprintf(buf + offset, remained - offset, 
				"| %-14s | %-20s| %10s / %10s | %14.2lf %% |\n",
				infos[i].device, infos[i].mountpoint, tmp, convert_unit(disk_total),
				get_proportion(disk_used, disk_total));
			free(tmp);
		}
	}
	offset += snprintf(buf + offset, remained - offset,
		"=====================================================================================\n");
	offset += snprintf(buf + offset, remained - offset, "%s", "\033[0m");
	return buf;
}

Module *get_module() {
    static Module disk_module = {
        .name = "disk",
        .respond_data = respond_disk_usage
    };
    return &disk_module;
}
