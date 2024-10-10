#ifndef INCLUDE_FI_EXT_H
#define INCLUDE_FI_EXT_H

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#define DLL_PREFIX extern "C"
#else
#define DLL_PREFIX
#endif

#if defined(_MSC_VER)
#define DLL_EXPORT __declspec(dllexport) DLL_PREFIX
#define DLL_IMPORT __declspec(dllimport) DLL_PREFIX
#elif defined(__GNUC__)
#define DLL_EXPORT __attribute__((visibility("default"))) DLL_PREFIX
#define DLL_IMPORT DLL_PREFIX
#endif

typedef void* dll_handle;
inline static char* get_shared_lib_name(const char* lib_name)
{
    size_t length = strlen(lib_name);
    char* file_name = NULL;
#ifdef __linux
    const char* file_ext = ".so";
    length += 3;
#else
    const char* file_ext = ".dll";
    length += 4;
#endif
    file_name = calloc(length + 1, sizeof(char));
    strcpy(file_name, lib_name);
    strcat(file_name, file_ext);

    return file_name;
}

#endif // INCLUDE_FI_EXT_H
