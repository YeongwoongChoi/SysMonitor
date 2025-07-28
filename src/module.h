#ifndef MODULE_H
#define MODULE_H
#include "common.h"

typedef void (*log_function)(char **, int *, const char *, const char *, ...);

typedef struct {
    const char *name;
    void (*initialize_module)(log_function logger);
    char* (*respond_data)();
} Module;
#endif
