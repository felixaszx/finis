#ifndef GRAPHICS_SHADER_HPP
#define GRAPHICS_SHADER_HPP

#include <map>

#include "tools.hpp"
#include "graphics.hpp"
#include "slang.h"

namespace fi::graphics
{
    class Shader : private GraphicsObject
    {
      private:
        vk::ShaderModule shader_{};
        vk::PipelineShaderStageCreateInfo stage_info_{};
        std::vector<std::string> desc_in{};
        std::vector<std::pair<uint32_t, uint32_t>> desc_sets_{};
        std::vector<vk::DescriptorSetLayoutBinding> desc_bindings_{};

        uint32_t push_constant_idx_ = -1;
        vk::PushConstantRange push_range_{};

      public:
        static inline const char* const ENTRY_POINT_ = "main";
        static slang::IGlobalSession& get_global_session();

        Shader(const std::filesystem::path& shader_file, const std::filesystem::path& include_path = "");
        ~Shader();

        [[nodiscard]] std::vector<std::string> get_desc_in() const { return desc_in; }
    };
}; // namespace fi::graphics

#endif // GRAPHICS_SHADER_HPP
