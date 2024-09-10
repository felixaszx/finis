#ifndef GRAPHICS_SHADER_HPP
#define GRAPHICS_SHADER_HPP

#include <map>
#include <fstream>

#include <spirv_cross/spirv_reflect.hpp>

#include "tools.hpp"
#include "graphics.hpp"
#include "slang.h"

namespace fi::gfx
{
    namespace spvc = spirv_cross;
    struct shader : private graphcis_obj
    {
        vk::ShaderModule module_{};
        std::vector<std::string> entrys_{};
        std::vector<vk::PipelineShaderStageCreateInfo> stage_infos_{};

        std::vector<std::string> push_const_names_{};
        std::vector<std::vector<std::string>> desc_names_{};
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> desc_sets_{};

        shader(const std::filesystem::path& shader_file, const std::filesystem::path& include_path = "");
        ~shader();
    };
}; // namespace fi::gfx

#endif // GRAPHICS_SHADER_HPP
