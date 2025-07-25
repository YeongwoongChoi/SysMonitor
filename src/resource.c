#include "resource.h"

CPUStat read_cpu_stat() {
    CPUStat stat = {0, };
    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return stat;
    fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", 
        &(stat.user), &(stat.nice), &(stat.system), &(stat.idle), &(stat.iowait),
        &(stat.irq), &(stat.softirq), &(stat.steal), &(stat.guest), &(stat.guest_nice));
    fclose(f);
    return stat;
}

double get_cpu_usage() {
    CPUStat prev = read_cpu_stat();
    usleep(300000);
    CPUStat curr = read_cpu_stat();

    unsigned long long idle = (curr.idle + curr.iowait) - (prev.idle + prev.iowait);

    unsigned long long active = (curr.user + curr.nice + curr.system + 
        curr.irq + curr.softirq + curr.steal) - (prev.user + prev.nice + prev.system + 
        prev.irq + prev.softirq + prev.steal);
    
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
