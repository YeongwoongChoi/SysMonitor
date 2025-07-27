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

MemStat read_mem_stat() {
    MemStat stat = {0, };
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f)
        return stat;

    char buf[128], key[64];
    unsigned long long value;
    while (fgets(buf, sizeof(buf), f)) {
        sscanf(buf, "%63s %llu", key, &value);
        value *= (1 << 10);
        if (!strcmp(key, "MemTotal:"))
            stat.mem_total = value;
        else if (!strcmp(key, "MemFree:"))
            stat.mem_free = value;
        else if (!strcmp(key, "MemAvailable:"))
            stat.mem_available = value;
        else if (!strcmp(key, "Buffers:"))
            stat.buffers = value;
        else if (!strcmp(key, "Cached:"))
            stat.cached = value;
        else if (!strcmp(key, "Shmem:"))
            stat.shared = value;
    }
    fclose(f);
    return stat;
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

double get_proportion(unsigned long long size, unsigned long long total) {
    return total ? 100.0 * ((double)size / total): 0.0;
}

