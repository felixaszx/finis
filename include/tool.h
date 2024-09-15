#ifndef INCLUDE_TOOL_H
#define INCLUDE_TOOL_H

#define GET_NEW(_0, _1, NEW, ...) NEW
#define NEW_0(type)               ((type*)calloc(1, sizeof(type)))
#define NEW_1(type, size)         ((type*)calloc(size, sizeof(type)))
#define new(...)                  GET_NEW(__VA_ARGS__, NEW_1, NEW_0)(__VA_ARGS__)
#define delete(ptr) \
    free(ptr);      \
    ptr = NULL

#endif // INCLUDE_TOOL_H
