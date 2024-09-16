#ifndef INCLUDE_TOOL_H
#define INCLUDE_TOOL_H

#include <stdlib.h>
#include <time.h>

#include <glib-2.0/glib.h>

#define nullptr NULL

#define GET_CNEW(_0, _1, CNEW, ...) CNEW
#define CNEW_0(type)                ((type*)calloc(1, sizeof(type)))
#define CNEW_1(type, size)          ((type*)calloc(size, sizeof(type)))
#define alloc(...)                  GET_CNEW(__VA_ARGS__, CNEW_1, CNEW_0)(__VA_ARGS__)
#define ffree(ptr) \
    if (ptr)       \
    {              \
        free(ptr); \
    }              \
    ptr = NULL

#define new(type, ...) (type*)new_##type(__VA_ARGS__)
#define delete(type, obj)       \
    release_##type((type*)obj); \
    ffree(obj)

// provide new, init and release type
#define DEFINE_OBJ(type, ...)                 \
    type* new_##type(__VA_ARGS__);            \
    void init_##type(type* obj, __VA_ARGS__); \
    void release_##type(type* obj)
// provide a bit more readable interface, not recommanded for using
#define DEFINE_OBJ_FUNC0(type, rt, func)      rt type_##func(type* obj)
#define DEFINE_OBJ_FUNC1(type, rt, func, ...) rt type_##func(type* obj, __VA_ARGS__)

struct timespec ms_sleep(size_t ms);

#endif // INCLUDE_TOOL_H