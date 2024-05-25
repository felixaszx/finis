#include "extensions/loader.hpp"

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

const ImageFunctions ExtensionLoader::load_image_funcs(const std::string& image_func_getter) const
{
    return dl_.get<ImageFunctions()>(image_func_getter)();
}

const SwapchainFunctions ExtensionLoader::load_swapchain_funcs(const std::string& swapchain_func_getter) const
{

    return dl_.get<SwapchainFunctions()>(swapchain_func_getter)();
}

const PassFunctions ExtensionLoader::load_pass_funcs(const std::string& pass_func_getter) const
{
    return dl_.get<PassFunctions()>(pass_func_getter)();
}
