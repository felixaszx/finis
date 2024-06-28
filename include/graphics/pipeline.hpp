#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>

#include "graphics.hpp"

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

    struct CombinedPipeline : public vk::Pipeline, //
                              public vk::PipelineLayout
    {
        vk::PipelineBindPoint type_{};

        bool operator==(const vk::Pipeline& target) { return *this == target; }
    };

    class PipelineMgr : private GraphicsObject
    {
      public:
      private:
        CombinedPipeline prev_{};

        std::vector<vk::PipelineLayout> layouts_{};
        std::vector<vk::Pipeline> pipelines_{};

      public:
        size_t build_pipeline_layout(const vk::PipelineLayoutCreateInfo& info);
        CombinedPipeline build_pipeline(size_t layout_idx, const vk::GraphicsPipelineCreateInfo& info);
        CombinedPipeline build_pipeline(size_t layout_idx, const vk::ComputePipelineCreateInfo& info);

        void bind_pipeline(vk::CommandBuffer cmd, const CombinedPipeline& pipeline);
        void bind_descriptor_sets(vk::CommandBuffer cmd, const vk::ArrayProxy<const vk::DescriptorSet>& sets,
                                  uint32_t first_set = 0, const vk::ArrayProxy<const uint32_t>& dynamic_offsets = {});
    };
}; // namespace fi

#endif // GRAPHICS_PIPELINE_HPP
