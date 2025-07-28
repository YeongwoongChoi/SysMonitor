#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;
} CPUStat;

typedef struct {
    unsigned long long mem_total;
    unsigned long long mem_free;
    unsigned long long mem_available;
    unsigned long long buffers;
    unsigned long long cached;
    unsigned long long shared;
} MemStat;

typedef struct {
    char device[32];
    char mountpoint[32];
    char filesystem[32];
} DiskInfo;

int is_filesystem_included(const char *filesystem);

long get_core_count();
CPUStat *read_cpu_stats(const int core_count);
MemStat read_mem_stat();
DiskInfo *read_disk_info(int *count);
double get_cpu_usage(CPUStat prev, CPUStat curr);
double get_proportion(unsigned long long size, unsigned long long total);
#endif
