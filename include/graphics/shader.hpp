#ifndef GRAPHICS2_SHADER_HPP
#define GRAPHICS2_SHADER_HPP

#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>

#include "graphics.hpp"
#include "tools.hpp"

namespace fi
{
    class ShaderModule : public vk::ShaderModule,                  //
                         public vk::PipelineShaderStageCreateInfo, //
                         private GraphicsObject                    //
    {
      private:
        std::string file_name_ = "";
        inline static shaderc::Compiler compiler_{};

      public:
        ShaderModule() = default;
        ShaderModule(const std::string& file_name, //
                     vk::ShaderStageFlagBits stage);
        ~ShaderModule();

        vk::PipelineShaderStageCreateInfo& info() { return *this; }
        void reset(const std::string& file_name, //
                   vk::ShaderStageFlagBits stage);
        void reset();
    };
}; // namespace fi

#endif // GRAPHICS2_SHADER_HPP
