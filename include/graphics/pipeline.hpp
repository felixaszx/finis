#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include "graphics.hpp"
#include "prim_res.hpp"
#include "shader.hpp"
#include "extensions/cpp_defines.hpp"

namespace fi::gfx
{
    struct proxy_pipeline : protected graphcis_obj
    {
        virtual ~proxy_pipeline() = default;
    };

    struct gfx_pipeline : protected graphcis_obj
    {
      protected:
        void config();

      public:
        gfx::shader* shader_ref_ = nullptr;
        std::vector<pipeline_pkg> pkgs_{};
        std::unordered_map<vk::DescriptorType, uint32_t> desc_sizes_{};

        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};
        std::vector<vk::DescriptorSetLayout> set_layouts_{};
        std::vector<vk::DescriptorSet> sets_{};

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

        virtual ~gfx_pipeline() = default;

        virtual void construct() = 0;
        virtual void setup_desc_set(vk::DescriptorPool pool) = 0;
    };

    struct cmp_pipeline : protected graphcis_obj
    {
        gfx::shader* shader_ref_ = nullptr;
        vk::PipelineLayout layout_{};

        vk::Pipeline pipeline_{};
        std::vector<pipeline_pkg> pkgs_;
        std::vector<vk::DescriptorSetLayout> set_layouts_;
        std::vector<vk::DescriptorSet> sets_{};

        virtual ~cmp_pipeline() = default;

        virtual void construct() = 0;
        virtual void setup_desc_set(vk::DescriptorPool pool) = 0;
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PIPELINE_HPP