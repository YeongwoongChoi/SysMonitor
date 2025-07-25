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
