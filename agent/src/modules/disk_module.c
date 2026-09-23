#include "module.h"
#include "resource.h"

char *respond_disk_usage(const char* request_type) {
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

	struct statvfs stat;
	if (strstr(request_type, ":json")) {
		for (int i = 0; i < count; ++i) {
			if (!strcmp(infos[i].mountpoint, "/")) {
				if (!statvfs(infos[i].mountpoint, &stat)) {
					unsigned long long disk_total = (unsigned long long)stat.f_blocks * (unsigned long long)stat.f_frsize;
					unsigned long long disk_free = (unsigned long long)stat.f_bfree * (unsigned long long)stat.f_frsize;
					unsigned long long disk_used = disk_total - disk_free;
					double main_percent = get_proportion(disk_used, disk_total);
					offset += snprintf(buf + offset, remained - offset, "{\"type\": \"disk\", \"main_used_percent\": %.2lf, \"devices\": [", 
                       main_percent);
					break;
				}

			}
		}
		for (int i = 0; i < count; ++i) {
			if (!statvfs(infos[i].mountpoint, &stat)) {				
				unsigned long long disk_total = (unsigned long long)stat.f_blocks * (unsigned long long)stat.f_frsize;
				unsigned long long disk_free = (unsigned long long)stat.f_bfree * (unsigned long long)stat.f_frsize;
				unsigned long long disk_used = disk_total - disk_free;

				char *tmp = strdup(convert_unit(disk_used));
				offset += snprintf(buf + offset, remained - offset, "{");
				offset += snprintf(buf + offset, remained - offset, "\"device\": \"%s\", ", infos[i].device);
				offset += snprintf(buf + offset, remained - offset, "\"mountpoint\": \"%s\", ", infos[i].mountpoint);
				offset += snprintf(buf + offset, remained - offset, "\"used\": \"%s\", ", tmp);
				offset += snprintf(buf + offset, remained - offset, "\"total\": \"%s\", ", convert_unit(disk_total));
				offset += snprintf(buf + offset, remained - offset, "\"percent\": %.2lf", get_proportion(disk_used, disk_total));
				offset += snprintf(buf + offset, remained - offset, "}%s", (i == count - 1) ? "": ", ");
				free(tmp);
			}
		}
		offset += snprintf(buf + offset, remained - offset, "]}");
	}
	else {
		offset += snprintf(buf + offset, remained - offset,
						"\033[0;94m%-99s\
						\n===================================================================================================\n",
						"[SysMonitor] Disk Usages");
		offset += snprintf(buf + offset, remained - offset,
						"| %-28s |    %-16s |      %9s          |  %-15s |\
						\n===================================================================================================\n",
						"Device", "Mounted at", "Size", "Proportion (%)");

		struct statvfs stat;
		for (int i = 0; i < count; ++i) {
			if (!statvfs(infos[i].mountpoint, &stat)) {
				unsigned long long disk_total = (unsigned long long)stat.f_blocks * (unsigned long long)stat.f_frsize;
				unsigned long long disk_free = (unsigned long long)stat.f_bfree * (unsigned long long)stat.f_frsize;
				unsigned long long disk_used = disk_total - disk_free;

				char *tmp = strdup(convert_unit(disk_used));
				offset += snprintf(buf + offset, remained - offset, 
								"| %-28s | %-20s| %10s / %10s | %14.2lf %% |\n",
								infos[i].device, infos[i].mountpoint, tmp, convert_unit(disk_total),
								get_proportion(disk_used, disk_total));
				free(tmp);
			}
		}
		offset += snprintf(buf + offset, remained - offset,
				"===================================================================================================\n");
		offset += snprintf(buf + offset, remained - offset, "%s", "\033[0m");
	}
	free(infos);
	return buf;
}

Module *get_module() {
	static Module disk_module = {
		.name = "disk",
		.respond_data = respond_disk_usage
	};
	return &disk_module;
}
