#ifndef INCLUDE_TOOL_H
#define INCLUDE_TOOL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdatomic.h>
#include <pthread.h>

#define nullptr NULL

#define GET_ALLOC(_0, _1, ALLOC, ...) ALLOC
#define ALLOC_0(type)                 (type*)malloc_zero(sizeof(type))
#define ALLOC_1(type, count)          (type*)malloc_zero(count * sizeof(type))
#define alloc(...)                    GET_ALLOC(__VA_ARGS__, ALLOC_1, ALLOC_0)(__VA_ARGS__)
#define ffree(ptr) \
    if (ptr)       \
    {              \
        free(ptr); \
    }              \
    ptr = NULL

#define to_kb(count) (1024 * count)
#define to_mb(count) (1024 * to_kb(count))
#define to_gb(count) (1024 * to_mb(count))

// provide new, init and release type
#define DEFINE_OBJ(type, ...)                        \
    type* new_##type();                              \
    type* construct_##type(type* this, __VA_ARGS__); \
    void destroy_##type(type* this)
#define DEFINE_OBJ_DEFAULT(type, ...) \
    type* new_##type();               \
    type* construct_##type(type* this);\
    void destroy_##type(type* this)

#define IMPL_OBJ_NEW(type, ...) \
    type* new_##type()          \
    {                           \
        return alloc(type);     \
    }                           \
    type* construct_##type(type* this, __VA_ARGS__)
#define IMPL_OBJ_NEW_DEFAULT(type) \
    type* new_##type()             \
    {                              \
        return alloc(type);        \
    }                              \
    type* construct_##type(type* this)
#define IMPL_OBJ_DELETE(type) void destroy_##type(type* this)
#define IMPL_OBJ_DELETE_DEFAULT(type) \
    void destroy_##type(type* this)   \
    {                                 \
    }

#define new(type, ...) (type*)construct_##type(new_##type(), __VA_ARGS__)
#define delete(type, obj)       \
    destroy_##type((type*)obj); \
    ffree(obj)

typedef char byte;
typedef size_t byte_offset;

struct timespec ms_sleep(size_t ms);

static inline void* malloc_zero(size_t size)
{
    void* ptr = malloc(size);
    memset(ptr, 0x0, size);
    return ptr;
}

#endif // INCLUDE_TOOL_H