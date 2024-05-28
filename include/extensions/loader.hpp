#ifndef INCLUDE_EXT_LOADER_HPP
#define INCLUDE_EXT_LOADER_HPP

#include <string>
#include <boost/dll.hpp>
#include "defines.h"

// the loading extension must be wirtten as a function
class ExtensionLoader
{
  private:
    boost::dll::shared_library dl_;

  public:
    ExtensionLoader(const std::string& dl_name);
    [[nodiscard]] const bool valid() const;

    [[nodiscard]] const BufferFunctions load_buffer_funcs(
        const std::string& buffer_func_getter = "buffer_func_getter") const;

    [[nodiscard]] const ImageFunctions load_image_funcs(
        const std::string& image_func_getter = "image_func_getter") const;

    // Pass extension should contain states.
    [[nodiscard]] const PassStates load_pass_states( //
        const std::string& pass_state_getter = "pass_state_getter") const;

    // scene manager extension should contain states.
    [[nodiscard]] const SceneManagerStates load_scene_manager_states( //
        const std::string& scene_manager_state_getter = "scene_manager_state_getter") const;
};

#endif // INCLUDE_EXT_LOADER_HPP
