#include "resource.h"

inline long get_core_count() { return sysconf(_SC_NPROCESSORS_ONLN); }

CPUStat *read_cpu_stats(const int core_count) {
    CPUStat *stats = (CPUStat *)malloc(sizeof(CPUStat) * (core_count + 1));
    memset(stats, 0, sizeof(CPUStat) * (core_count + 1));
    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return stats;

    char buf[256];
    int core_num = 1;

    while (fgets(buf, sizeof(buf), f)) {
        CPUStat *cpu = NULL;

        if (!strncmp(buf, "cpu ", 4))
            cpu = &stats[0];
        else if (!strncmp(buf, "cpu", 3)) {
            cpu = &stats[core_num];
            core_num++;
        }
        
        if (cpu) {
            sscanf(buf, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                &(cpu->user), &(cpu->nice), &(cpu->system), &(cpu->idle),
                &(cpu->iowait), &(cpu->irq), &(cpu->softirq), &(cpu->steal), 
                &(cpu->guest), &(cpu->guest_nice));
        }
        if (core_num >= core_count)
            break;
    }
    fclose(f);
    return stats;
}

double get_cpu_usage(CPUStat prev, CPUStat curr) {
    unsigned long long idle = (curr.idle + curr.iowait) - (prev.idle + prev.iowait);
    unsigned long long active =
        (curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal) -
        (prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal);

    unsigned long long total = idle + active;
    if (total == 0)
        return 0.0;

    return 100.0 * active / total;
}

double get_mem_usage() {
    FILE *f = fopen("/proc/meminfo", "r");
    unsigned long long mem_total = 0, mem_available = 0;

    char buf[128], key[64];
    unsigned long long value;
    while (fgets(buf, sizeof(buf), f)) {
        sscanf(buf, "%63s %llu", key, &value);
        if (!strcmp(key, "MemTotal:"))
            mem_total = value;
        else if (!strcmp(key, "MemAvailable:"))
            mem_available = value;

        if (mem_total && mem_available)
            break;
    }
    fclose(f);

    if (mem_total == 0)
        return 0.0;

    return 100.0 * (1.0 - ((double)mem_available / mem_total));
}

double get_disk_usage(const char *path) {
    struct statvfs stat;

    if (statvfs(path, &stat) != 0)
        return 0.0;

    unsigned long long total = (unsigned long long)stat.f_blocks * stat.f_frsize;
    unsigned long long free = (unsigned long long)stat.f_bfree * stat.f_frsize;

    return 100.0 * (1.0 - ((double)free / total));
}
