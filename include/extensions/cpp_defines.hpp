#ifndef EXTENSIONS_CPP_DEFINES_HPP
#define EXTENSIONS_CPP_DEFINES_HPP

#include <memory>
#include <boost/config.hpp>
#include <boost/dll.hpp>

#include "tools.hpp"

#define EXTENSION_API                           extern "C" BOOST_SYMBOL_EXPORT
#define GET_EXPORT_EXTENSION(_0, _1, FUNC, ...) FUNC
#define EXPORT_EXTENSION_0(Type)           \
    EXTENSION_API void* load_extension_0() \
    {                                      \
        return (void*)(new Type);          \
    }
#define EXPORT_EXTENSION_1(Type, idx)          \
    EXTENSION_API void* load_extension_##idx() \
    {                                          \
        return (void*)(new Type);              \
    }
#define REGISTER_EXT(...) GET_EXPORT_EXTENSION(__VA_ARGS__, EXPORT_EXTENSION_1, EXPORT_EXTENSION_0)(__VA_ARGS__)

#define GET_CUSTOM_EXPORT_EXT(_0, _1, FUNC, ...) FUNC
#define CUSTOM_EXPORT_EXT_0(obj)           \
    EXTENSION_API void* load_extension_0() \
    {                                      \
        return obj;                        \
    }
#define CUSTOM_EXPORT_EXT_1(obj, idx)          \
    EXTENSION_API void* load_extension_##idx() \
    {                                          \
        return obj;                            \
    }
#define REGISTER_EXT2(...) GET_EXPORT_EXTENSION(__VA_ARGS__, CUSTOM_EXPORT_EXT_1, CUSTOM_EXPORT_EXT_0)(__VA_ARGS__)

#endif // EXTENSIONS_CPP_DEFINES_HPP