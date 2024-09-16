#ifndef INCLUDE_TOOL_H
#define INCLUDE_TOOL_H

#include <stdlib.h>
#include <time.h>

#include <glib-2.0/glib.h>

#define nullptr NULL

#define GET_ALLOC(_0, _1, ALLOC, ...) ALLOC
#define ALLOC_0(type)                 calloc(1, sizeof(type))
#define ALLOC_1(type, size)           calloc(size, sizeof(type))
#define alloc(...)                    GET_ALLOC(__VA_ARGS__, ALLOC_1, ALLOC_0)(__VA_ARGS__)
#define ffree(ptr) \
    if (ptr)       \
    {              \
        free(ptr); \
    }              \
    ptr = NULL

// provide new, init and release type
#define DEFINE_OBJ_DEFAULT(type) \
    type* new_##type();          \
    void init_##type(type* obj); \
    void release_##type(type* obj)
#define DEFINE_OBJ(type, ...)                 \
    type* new_##type(__VA_ARGS__);            \
    void init_##type(type* obj, __VA_ARGS__); \
    void release_##type(type* obj)
// provide a bit more readable interface, not recommanded for using
#define DEFINE_OBJ_FUNC0(type, rt, func)      rt type_##func(type* obj)
#define DEFINE_OBJ_FUNC1(type, rt, func, ...) rt type_##func(type* obj, __VA_ARGS__)
#define IMPL_OBJ_DEFAULT_NEW(type) \
    type* new_##type()             \
    {                              \
        type* obj = alloc(type);   \
        init_##type(obj);          \
        return obj;                \
    }

#define new(type, ...) (type*)new_##type(__VA_ARGS__)
#define delete(type, obj)       \
    release_##type((type*)obj); \
    ffree(obj)

typedef char byte;
struct timespec ms_sleep(size_t ms);

static inline size_t to_kb(size_t count)
{
    return count * 1024;
}

static inline size_t to_mb(size_t count)
{
    return to_kb(count) * 1024;
}

static inline size_t to_gb(size_t count)
{
    return to_mb(count) * 1024;
}

#endif // INCLUDE_TOOL_H