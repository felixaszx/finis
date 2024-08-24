#ifndef ENGINE_PIPELINE_HPP
#define ENGINE_PIPELINE_HPP

#include "graphics/graphics.hpp"
#include "graphics/shader.hpp"
#include "extensions/cpp_defines.hpp"
#include "tools.hpp"

namespace fi
{
    struct ProxyPipelineBase : public Extension, //
                               protected GraphicsObject
    {
        virtual ~ProxyPipelineBase() = default;
    };

    struct GraphicsPipelineBase : public Extension, //
                                  protected GraphicsObject
    {
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

        virtual ~GraphicsPipelineBase() = default;

        virtual void get_pipeline_info(uint32_t width, uint32_t height) = 0;
        virtual std::vector<vk::Image>& get_images() = 0;
        virtual std::vector<vk::ImageView>& get_image_views() = 0;
        virtual std::vector<uint32_t> get_color_atchm_idx() { return {}; };
        virtual uint32_t get_depth_atchm_idx() { return -1; };
        virtual uint32_t get_stencil_atchm_idx() { return -1; };
        virtual uint32_t get_ds_atchm_idx() { return -1; };
    };

    struct ComputePipelineBase : public Extension, //
                                 private GraphicsObject
    {
        std::vector<ShaderModule> shaders_{};

        vk::PipelineShaderStageCreateInfo shader_stages_{};
        vk::ComputePipelineCreateInfo create_info_{};

        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};

        virtual ~ComputePipelineBase() = default;
    };
}; // namespace fi

#endif // ENGINE_PIPELINE_HPP