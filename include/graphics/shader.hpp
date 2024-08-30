#ifndef GRAPHICS_SHADER_HPP
#define GRAPHICS_SHADER_HPP

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

      public:
        static inline const char* const ENTRY_POINT_ = "main";
        static slang::IGlobalSession& get_global_session();

        Shader(const std::filesystem::path& shader_file, const std::filesystem::path& include_path = "");
        ~Shader();

        [[nodiscard]] std::vector<std::string> get_desc_in() const { return desc_in; }
    };
}; // namespace fi::graphics

#endif // GRAPHICS_SHADER_HPP
