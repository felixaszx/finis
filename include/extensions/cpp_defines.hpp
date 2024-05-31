#ifndef EXTENSIONS_CPP_DEFINES_HPP
#define EXTENSIONS_CPP_DEFINES_HPP

#include <memory>
#include <boost/config.hpp>

#define LOARDER_FUNC_NAME "load_extension"
#define EXTENSION_API     extern "C" BOOST_SYMBOL_EXPORT
#define EXPORT_EXTENSION(Type)                                \
    EXTENSION_API Extension* load_extension(const char* path) \
    {                                                         \
        Type* ext = new Type;                                 \
        ext->id_ = path;                                      \
        return ext;                                           \
    }
#define bit_shift_left(bits) (1 << bits)
#define TRY_FUNC \
    try          \
    {

#define CATCH_BEGIN            \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what();
#define CATCH_END }
#define CATCH_FUNC             \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what(); \
    }

struct Extension
{
    std::string id_ = "";
    std::string usage_ = "";
    virtual ~Extension() = default;
};

#endif // EXTENSIONS_CPP_DEFINES_HPP
