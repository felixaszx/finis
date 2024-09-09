#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include "graphics.hpp"
#include "shader.hpp"
#include "extensions/cpp_defines.hpp"

namespace fi::gfx
{
    struct proxy_pipeline : public ext::base, //
                            protected graphcis_obj
    {
        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};
    };

    struct gfx_pipeline : public proxy_pipeline
    {
        vk::PipelineRenderingCreateInfo atchms_{};
        vk::PipelineVertexInputStateCreateInfo vtx_input_{};
        vk::PipelineInputAssemblyStateCreateInfo input_asm_{};
        vk::PipelineTessellationStateCreateInfo tessellation_{};
        vk::PipelineViewportStateCreateInfo viewport_{};
        vk::PipelineRasterizationStateCreateInfo rasterizer_{};
        vk::PipelineMultisampleStateCreateInfo multi_sample_{};
        vk::PipelineDepthStencilStateCreateInfo depth_stencil_{};
        vk::PipelineColorBlendStateCreateInfo color_blend_{};
        vk::PipelineDynamicStateCreateInfo dynamic_state_{};
    };

    struct cmp_pipeline : public proxy_pipeline
    {
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PIPELINE_HPP