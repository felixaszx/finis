
#include "fi_tool.h"

#ifdef __linux__
#include <threads.h>
#else
#include <Windows.h>
#endif

void ms_sleep(size_t ms)
{
#ifdef __linux__
    struct timespec t1 = {};
    struct timespec t2 = {};
    t1.tv_nsec = ms * 1000000;
    thrd_sleep(&t1, &t2);
#else
    Sleep(ms);
#endif
}