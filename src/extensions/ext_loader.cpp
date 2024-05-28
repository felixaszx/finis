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

const PassStates ExtensionLoader::load_pass_states(const std::string& pass_state_getter) const
{
    return dl_.get<PassStates()>(pass_state_getter)();
}
const SceneManagerStates ExtensionLoader::load_scene_manager_states(const std::string& scene_manager_state_getter) const
{
    return dl_.get<SceneManagerStates()>(scene_manager_state_getter)();
}
