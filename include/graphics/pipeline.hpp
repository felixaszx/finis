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
        virtual vk::Pipeline get_pipeline() = 0;
        virtual vk::PipelineLayout get_layout() = 0;
    };

    struct gfx_pipeline : public ext::base, //
                          protected graphcis_obj
    {
        vk::PipelineLayout layout_{};
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

        virtual vk::Pipeline get_pipeline() = 0;
        virtual vk::PipelineLayout get_layout() = 0;
    };

    struct cmp_pipeline : public ext::base, //
                          protected graphcis_obj
    {
        virtual vk::Pipeline get_pipeline() = 0;
        virtual vk::PipelineLayout get_layout() = 0;
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PIPELINE_HPP
