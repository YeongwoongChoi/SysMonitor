#ifndef MODULE_H
#define MODULE_H
#include "common.h"
#define TOTAL_MODULES 3

typedef struct {
    const char *name;
    char* (*respond_data)();
} Module;
#endif
