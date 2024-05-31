#ifndef EXTENSIONS_CPP_DEFINES_HPP
#define EXTENSIONS_CPP_DEFINES_HPP

#include <memory>
#include <concepts>
#include <boost/config.hpp>

#include "tools.hpp"

#define LOARDER_FUNC_NAME "load_extension"
#define EXTENSION_API     extern "C" BOOST_SYMBOL_EXPORT
#define EXPORT_EXTENSION(Type)                                \
    EXTENSION_API Extension* load_extension(const char* path) \
    {                                                         \
        Type* ext = new Type;                                 \
        ext->id_ = path;                                      \
        return ext;                                           \
    }

struct Extension
{
    std::string id_ = "";
    std::string description_ = "";
    virtual ~Extension() = default;

    enum Usage : uint64_t
    {
        // system usage
        SYSTEM_testing,
        SYSTEM_graphics
    };
    Usage usage_ = Usage::SYSTEM_testing;

    enum Component : uint64_t
    {
        // SYSTEM_testing
        TESTING_none = 0,

        // SYSTEM_graphics
        GRAPHICS_mouse_callback = bit_shift_left(0),

        // GAME_script
        SCRIPT_frame_update = bit_shift_left(0),
        SCRIPT_fix_update = bit_shift_left(1)
    };
    Component component_ = Component::TESTING_none;
};

#endif // EXTENSIONS_CPP_DEFINES_HPP
