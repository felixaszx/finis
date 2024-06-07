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

struct MeshMemory;

struct Extension
{
  public:
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
        GRAPHICS_model_loading = bit_shift_left(0),

        // GAME_script
        SCRIPT_frame_update = bit_shift_left(0),
        SCRIPT_fix_update = bit_shift_left(1)
    };
    Component component_ = Component::TESTING_none;

    virtual void load_model(MeshMemory& mesh_memory, std::string& name, const std::string& path,
                            uint32_t max_instances) {};
};

#endif // EXTENSIONS_CPP_DEFINES_HPP
