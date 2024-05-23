#include "ext_loader.hpp"

ExtensionLoader::ExtensionLoader(const std::string& dl_name)
    : dl_(dl_name)
{
}

const bool ExtensionLoader::valid() const
{
    return dl_.is_loaded();
}

const BufferFunctions ExtensionLoader::load_buffer_funcs(const std::string& buffer_func_getter) const
{
    return dl_.get<BufferFunctions()>(buffer_func_getter)();
}
