#include "tools.hpp"
#include "graphics/pipeline.hpp"
#include "extensions/cpp_defines.hpp"

using namespace fi;
struct pipeline : public gfx::gfx_pipeline
{
    gfx::shader shader_;
    uint32_t total_tex_ = 0;

    std::vector<std::vector<std::byte>> push_consts_{};
    std::vector<vk::Buffer> uniforms_{};
    std::vector<vma::Allocation> allocs_{};

    pipeline()
        : shader_("res/shaders/spvs/test.spv")
    {
        shader_ref_ = &shader_;
    }

    void construct() override
    {
        for (auto& pkg : pkgs_)
        {
            if (pkg.tex_arr_)
            {
                total_tex_ += pkg.tex_arr_->desc_infos_.size();
            }
        }

        for (auto& bindings : shader_.desc_sets_)
        {
            vk::DescriptorSetLayoutCreateInfo set_layout_info{};

            for (auto& b : bindings)
            {
                if (b.descriptorType == vk::DescriptorType::eCombinedImageSampler && b.descriptorCount == -1)
                {
                    b.descriptorCount = total_tex_;
                }
                if (desc_sizes_.contains(b.descriptorType))
                {
                    desc_sizes_[b.descriptorType]++;
                }
                else
                {
                    desc_sizes_[b.descriptorType] = b.descriptorCount;
                }
            }

            set_layout_info.setBindings(bindings);
            set_layouts_.push_back(device().createDescriptorSetLayout(set_layout_info));
        }

        config();

        std::vector<vk::Format> color_formats_(shader_.atchm_count_, vk::Format::eR32G32B32A32Sfloat);
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

        std::vector<vk::PipelineColorBlendAttachmentState> blend_states(shader_.atchm_count_);
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

    std::vector<vk::DescriptorSet> setup_desc_set(vk::DescriptorPool pool) override
    {
        vk::DescriptorSetAllocateInfo alloc_info{};
        alloc_info.descriptorPool = pool;
        alloc_info.setSetLayouts(set_layouts_);
        std::vector<vk::DescriptorSet> sets = device().allocateDescriptorSets(alloc_info);

        for (uint32_t s = 0; s < shader_.desc_sets_.size(); s++)
        {
            for (auto& binding : shader_.desc_sets_[s])
            {
                switch (binding.descriptorType)
                {
                    case vk::DescriptorType::eCombinedImageSampler:
                    {
                        break;
                    }
                    case vk::DescriptorType::eStorageImage:
                    {
                        break;
                    }
                    case vk::DescriptorType::eUniformBuffer:
                    {
                        break;
                    }
                    case vk::DescriptorType::eStorageBuffer:
                    {
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        return sets;
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