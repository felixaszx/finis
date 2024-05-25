#ifndef INCLUDE_SHADER_MODULE_HPP
#define INCLUDE_SHADER_MODULE_HPP

#include <shaderc/shaderc.hpp>

#include "vk_base.hpp"

class ShaderModule : private VkObject,        //
                     public vk::ShaderModule, //
                     public vk::PipelineShaderStageCreateInfo
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

#endif // INCLUDE_SHADER_MODULE_HPP
