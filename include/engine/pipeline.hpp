#ifndef ENGINE_PIPELINE_HPP
#define ENGINE_PIPELINE_HPP

#include "graphics/graphics.hpp"
#include "graphics/shader.hpp"
#include "extensions/cpp_defines.hpp"
#include "tools.hpp"

namespace fi
{
    struct GraphicsPipelineBase : public Extension, //
                                  protected GraphicsObject
    {
        std::vector<vk::Image> images_{};
        std::vector<vma::Allocation> allocations_{};
        std::vector<vk::ImageView> views_{};
        std::vector<vk::RenderingAttachmentInfo> atchm_infos_{};

        ShaderModule vs_{};
        ShaderModule gs_{};
        ShaderModule fs_{};

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages_{};
        std::vector<vk::PipelineColorBlendAttachmentState> atchm_blends_{};
        std::vector<vk::DynamicState> dynamic_states_{};

        vk::PipelineRenderingCreateInfo pipeline_rendering_{};
        vk::PipelineInputAssemblyStateCreateInfo input_asm_{};
        vk::PipelineTessellationStateCreateInfo tesselation_info_{};
        vk::PipelineViewportStateCreateInfo view_port_{};
        vk::PipelineRasterizationStateCreateInfo rasterizer_{};
        vk::PipelineMultisampleStateCreateInfo multi_sample_{};
        vk::PipelineDepthStencilStateCreateInfo pso_ds_info_{};
        vk::PipelineColorBlendStateCreateInfo blend_states_{};
        vk::PipelineDynamicStateCreateInfo dynamic_state_info_{};
        vk::GraphicsPipelineCreateInfo create_info{};

        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};
        vk::RenderingInfo rendering_info_{};

        virtual void get_pipeline_info(uint32_t width, uint32_t height) = 0;
    };

    struct ComputePipelineBase : public Extension, //
                                 private GraphicsObject
    {
        std::vector<ShaderModule> shaders_{};

        vk::PipelineShaderStageCreateInfo shader_stages_{};
        vk::ComputePipelineCreateInfo create_info_{};

        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};
    };
}; // namespace fi

#endif // ENGINE_PIPELINE_HPP