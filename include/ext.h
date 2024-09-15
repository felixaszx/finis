#ifndef INCLUDE_EXT_H
#define INCLUDE_EXT_H

#if defined(_MSC_VER)
#define EXPORTED __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#else
#define EXPORTED __attribute__((dllexport))
#endif

#endif // INCLUDE_EXT_H
