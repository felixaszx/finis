#ifndef EXTENSIONS_CPP_DEFINES_HPP
#define EXTENSIONS_CPP_DEFINES_HPP

#include <memory>
#include <boost/config.hpp>
#include <boost/dll.hpp>

#include "tools.hpp"

#define EXTENSION_API                           extern "C" BOOST_SYMBOL_EXPORT
#define GET_EXPORT_EXTENSION(_0, _1, FUNC, ...) FUNC
#define EXPORT_EXTENSION_0(Type)                    \
    EXTENSION_API fi::ext::base* load_extension_0() \
    {                                               \
        auto* e = new Type;                         \
        e->id_ = std::to_string(size_t(e));         \
        return e;                                   \
    }
#define EXPORT_EXTENSION_1(Type, idx)                   \
    EXTENSION_API fi::ext::base* load_extension_##idx() \
    {                                                   \
        auto* e = new Type;                             \
        e->id_ = std::to_string(size_t(e));             \
        return e;                                       \
    }
#define EXPORT_EXTENSION(...) GET_EXPORT_EXTENSION(__VA_ARGS__, EXPORT_EXTENSION_1, EXPORT_EXTENSION_0)(__VA_ARGS__)

namespace fi::ext
{

    struct base
    {
        std::string id_ = "";
        std::string description_ = "";
        virtual ~base() = default;
    };
}; // namespace fi::ext

#endif // EXTENSIONS_CPP_DEFINES_HPP
