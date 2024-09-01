#ifndef EXTENSIONS_CPP_DEFINES_HPP
#define EXTENSIONS_CPP_DEFINES_HPP

#include <memory>
#include <boost/config.hpp>
#include <boost/dll.hpp>

#include "tools.hpp"

#define EXTENSION_API                           extern "C" BOOST_SYMBOL_EXPORT
#define GET_EXPORT_EXTENSION(_0, _1, FUNC, ...) FUNC
#define EXPORT_EXTENSION_0(Type)                    \
    EXTENSION_API fi::Extension* load_extension_0() \
    {                                               \
        auto* ext = new Type;                       \
        ext->id_ = std::to_string(size_t(ext));     \
        return ext;                                 \
    }
#define EXPORT_EXTENSION_1(Type, idx)                   \
    EXTENSION_API fi::Extension* load_extension_##idx() \
    {                                                   \
        auto* ext = new Type;                           \
        ext->id_ = std::to_string(size_t(ext));         \
        return ext;                                     \
    }
#define EXPORT_EXTENSION(...) GET_EXPORT_EXTENSION(__VA_ARGS__, EXPORT_EXTENSION_1, EXPORT_EXTENSION_0)(__VA_ARGS__)

namespace fi
{
    struct ext
    {
        std::string id_ = "";
        std::string description_ = "";
        virtual ~ext() = default;
    };
}; // namespace fi

#endif // EXTENSIONS_CPP_DEFINES_HPP
