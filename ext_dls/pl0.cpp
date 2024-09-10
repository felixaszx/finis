#include "tools.hpp"
#include "graphics/pipeline.hpp"
#include "extensions/cpp_defines.hpp"

static size_t count = 0;

using namespace fi;
struct pipeline : public gfx::gfx_pipeline
{
    gfx::shader shader_;
    uint32_t total_tex_;

    pipeline()
        : shader_("res/shaders/spvs/test.spv")
    {
        shader_ref_ = &shader_;
        for (auto& pkg : pkgs_)
        {
            if (pkg.tex_arr_)
            {
                total_tex_ += pkg.tex_arr_->desc_infos_.size();
            }
        }
    }

    void construct() override
    {
        for (auto& bindings : shader_.desc_sets_)
        {
            vk::DescriptorSetLayoutCreateInfo set_layout_info{};

            for (auto& b : bindings)
            {
                if (b.descriptorType == vk::DescriptorType::eCombinedImageSampler && b.descriptorCount == -1)
                {
                    b.descriptorCount = total_tex_;
                }
            }

            set_layout_info.setBindings(bindings);
            set_layouts_.push_back(device().createDescriptorSetLayout(set_layout_info));
        }

        vk::PipelineLayoutCreateInfo pl_layout_info{};
        pl_layout_info.setSetLayouts(set_layouts_);
        if (shader_.push_const_names_.size() > 0)
        {
        }
        layout_ = device().createPipelineLayout(pl_layout_info);

        static std::array<vk::Format, 4> color_formats_ = {vk::Format::eR32G32B32A32Sfloat, //
                                                           vk::Format::eR32G32B32A32Sfloat, //
                                                           vk::Format::eR32G32B32A32Sfloat,
                                                           vk::Format::eR32G32B32A32Sfloat};
        atchms_.setColorAttachmentFormats(color_formats_);
        atchms_.depthAttachmentFormat = vk::Format::eD24UnormS8Uint;
        atchms_.stencilAttachmentFormat = atchms_.depthAttachmentFormat;
        input_asm_.topology = vk::PrimitiveTopology::eTriangleList;
        viewport_.viewportCount = 1;
        viewport_.scissorCount = 1;
        rasterizer_.lineWidth = 1;
        rasterizer_.polygonMode = vk::PolygonMode::eFill;
        multi_sample_.rasterizationSamples = vk::SampleCountFlagBits::e1;
        depth_stencil_.depthWriteEnable = true;
        depth_stencil_.depthTestEnable = true;
        depth_stencil_.depthCompareOp = vk::CompareOp::eLess;

        static std::array<vk::PipelineColorBlendAttachmentState, 4> blend_states = {};
        for (auto& state : blend_states)
        {
            state.colorWriteMask = vk::ColorComponentFlagBits::eR | //
                                   vk::ColorComponentFlagBits::eG | //
                                   vk::ColorComponentFlagBits::eB | //
                                   vk::ColorComponentFlagBits::eA;
        }
        color_blend_.setAttachments(blend_states);

        static std::array<vk::DynamicState, 2> dynamic_states = {vk::DynamicState::eViewport,
                                                                 vk::DynamicState::eScissor};
        dynamic_state_.setDynamicStates(dynamic_states);

        vk::GraphicsPipelineCreateInfo create_info{};
        create_info.pNext = &atchms_;
        create_info.setStages(shader_.stage_infos_);
        create_info.pVertexInputState = &vtx_input_;
        create_info.pInputAssemblyState = &input_asm_;
        create_info.pTessellationState = &tessellation_;
        create_info.pViewportState = &viewport_;
        create_info.pRasterizationState = &rasterizer_;
        create_info.pMultisampleState = &multi_sample_;
        create_info.pDepthStencilState = &depth_stencil_;
        create_info.pColorBlendState = &color_blend_;
        create_info.pDynamicState = &dynamic_state_;
        create_info.layout = layout_;
        pipeline_ = device().createGraphicsPipelines(pipeline_cache(), create_info).value[0];
    }

    ~pipeline() override
    {
        for (auto& set_layout : set_layouts_)
        {
            device().destroyDescriptorSetLayout(set_layout);
        }
        device().destroyPipelineLayout(layout_);
        device().destroyPipeline(pipeline_);
    }
};

EXPORT_EXTENSION(pipeline);