#ifndef INCLUDE_FI_EXT_H
#define INCLUDE_FI_EXT_H

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <string>
#define FI_DLL_PREFIX extern "C"

inline static std::string get_shared_lib_name(const std::string& lib_name)
{
#ifdef __linux__
    std::string file_ext = ".so";
#else
    std::string file_ext = ".dll";
#endif
    return lib_name + file_ext;
}
#else
#define FI_DLL_PREFIX
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
#define FI_DLL_EXPORT FI_DLL_PREFIX __declspec(dllexport)
#else
#define FI_DLL_EXPORT FI_DLL_PREFIX __attribute__((__visibility__("default")))
#endif

typedef void* dll_handle;
inline static char* get_shared_lib_name(const char* lib_name)
{
    size_t length = strlen(lib_name);
    char* file_name = NULL;
#ifdef __linux__
    const char* file_ext = ".so";
    length += 3;
#else
    const char* file_ext = ".dll";
    length += 4;
#endif
    file_name = (char*)calloc(length + 1, sizeof(char));
    strcpy(file_name, lib_name);
    strcat(file_name, file_ext);

    return file_name;
}

#endif // INCLUDE_FI_EXT_H
