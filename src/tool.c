#include "fi_tool.h"

struct timespec ms_sleep(size_t ms)
{
    struct timespec t1 = {};
    struct timespec t2 = {};
    t1.tv_nsec = ms * 1000000;
    nanosleep(&t1, &t2);
    return t2;
}