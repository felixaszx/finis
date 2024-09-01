#ifndef GRAPHICS_SHADER_HPP
#define GRAPHICS_SHADER_HPP

#include <map>

#include <spirv_cross/spirv_reflect.hpp>

#include "tools.hpp"
#include "graphics.hpp"
#include "slang.h"

namespace fi::graphics
{
    namespace spvc = spirv_cross;
    class Shader : private GraphicsObject
    {
      private:
        vk::ShaderModule module_{};
        std::vector<std::string> entrys_{};
        std::vector<vk::PipelineShaderStageCreateInfo> stage_infos_{};

        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> desc_sets_{};
        std::vector<std::vector<std::string>> desc_names_{};
        std::vector<vk::PipelineColorBlendAttachmentState> atchm_blends_{};

        std::vector<std::string> push_const_names_{};

      public:
        static slang::IGlobalSession& get_global_session();

        Shader(const std::filesystem::path& shader_file, const std::filesystem::path& include_path = "");
        ~Shader();
    };
}; // namespace fi::graphics

#endif // GRAPHICS_SHADER_HPP