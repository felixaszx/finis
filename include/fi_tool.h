#ifndef INCLUDE_FI_TOOL_H
#define INCLUDE_FI_TOOL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>

#define nullptr NULL

#define GET_FI_ALLOC(_0, _1, ALLOC, ...) ALLOC
#define FI_ALLOC_0(type)                 (type*)malloc_zero(sizeof(type))
#define FI_ALLOC_1(type, count)          (type*)malloc_zero(count * sizeof(type))
#define fi_alloc(...)                    GET_FI_ALLOC(__VA_ARGS__, FI_ALLOC_1, FI_ALLOC_0)(__VA_ARGS__)
#define fi_free(ptr) \
    free(ptr);     \
    ptr = NULL

#define to_kb(count)      (1024 * count)
#define to_mb(count)      (1024 * to_kb(count))
#define to_gb(count)      (1024 * to_mb(count))
#define fi_size_of_arr(arr) (sizeof(arr) / sizeof(arr[0]))

// provide new, init and release type
#define DEFINE_OBJ(type, ...)                        \
    type* new_##type();                              \
    type* construct_##type(type* cthis, __VA_ARGS__); \
    void destroy_##type(type* cthis)
#define DEFINE_OBJ_DEFAULT(type)        \
    type* new_##type();                 \
    type* construct_##type(type* cthis); \
    void destroy_##type(type* cthis)

#define IMPL_OBJ_NEW(type, ...) \
    type* new_##type()          \
    {                           \
        return fi_alloc(type);     \
    }                           \
    type* construct_##type(type* cthis, __VA_ARGS__)
#define IMPL_OBJ_NEW_DEFAULT(type) \
    type* new_##type()             \
    {                              \
        return fi_alloc(type);        \
    }                              \
    type* construct_##type(type* cthis)
#define IMPL_OBJ_DELETE(type) void destroy_##type(type* cthis)
#define IMPL_OBJ_DELETE_DEFAULT(type) \
    void destroy_##type(type* cthis)   \
    {                                 \
    }

#define fi_new(type, ...) (type*)construct_##type(new_##type(), __VA_ARGS__)
#define fi_delete(type, obj)      \
    {                          \
        type* t_ptr = obj;     \
        destroy_##type(t_ptr); \
    }                          \
    fi_free(obj)

typedef unsigned char byte;
typedef size_t byte_offset;
typedef void T;

void ms_sleep(size_t ms);

static inline T* malloc_zero(size_t size)
{
    T* ptr = malloc(size);
    memset(ptr, 0x0, size);
    return ptr;
}

#endif // INCLUDE_FI_TOOL_H
