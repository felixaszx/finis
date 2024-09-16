#ifndef INCLUDE_EXT_H
#define INCLUDE_EXT_H

#if defined(_MSC_VER)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((dllexport))
#endif

#endif // INCLUDE_EXT_H
