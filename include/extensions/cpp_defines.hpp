#ifndef EXTENSIONS_CPP_DEFINES_HPP
#define EXTENSIONS_CPP_DEFINES_HPP

#include <memory>
#include <concepts>
#include <boost/config.hpp>
#include <boost/dll.hpp>

#include "tools.hpp"

#define LOARDER_FUNC_NAME "load_extension"
#define EXTENSION_API     extern "C" BOOST_SYMBOL_EXPORT
#define EXPORT_EXTENSION(Type)                  \
    EXTENSION_API Extension* load_extension()   \
    {                                           \
        auto* ext = new Type;                   \
        ext->id_ = std::to_string(size_t(ext)); \
        return ext;                             \
    }

namespace fi
{
    struct Extension
    {
      public:
        std::string id_ = "";
        std::string description_ = "";
        virtual ~Extension() = default;
    };
}; // namespace fi

#endif // EXTENSIONS_CPP_DEFINES_HPP
