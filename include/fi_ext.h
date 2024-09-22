#ifndef INCLUDE_FI_EXT_H
#define INCLUDE_FI_EXT_H

#include <dlfcn.h>

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

#endif // INCLUDE_FI_EXT_H
