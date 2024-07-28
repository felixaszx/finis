#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/buffer.hpp"
#include "graphics/render_mgr.hpp"
#include "graphics/animation_mgr.hpp"

struct GBufferPass : private fi::GraphicsObject
{
    fi::ShaderModule vs_{"res/shaders/vulkan0.vert", vk::ShaderStageFlagBits::eVertex};
    fi::ShaderModule fs_{"res/shaders/vulkan0.frag", vk::ShaderStageFlagBits::eFragment};
    std::vector<vk::PipelineShaderStageCreateInfo> shader_infos_ = {vs_, fs_};

    fi::CombinedPipeline pipeline_;
    vk::PipelineLayout p_layout_{};
    vk::PipelineVertexInputStateCreateInfo vtx_state_{};
    vk::PipelineInputAssemblyStateCreateInfo input_asm_{};
    vk::PipelineTessellationStateCreateInfo tesselation_info_{};
    vk::PipelineViewportStateCreateInfo view_port_{};
    vk::PipelineRasterizationStateCreateInfo rasterizer_{};
    vk::PipelineMultisampleStateCreateInfo multi_sample_{};
    vk::PipelineDepthStencilStateCreateInfo ds_info_{};
    vk::PipelineColorBlendStateCreateInfo blend_states_{};
    vk::PipelineDynamicStateCreateInfo dynamic_state_info_{};
    vk::PipelineRenderingCreateInfo render_info_{};

    std::vector<vk::Image> color_atchms_{};
    std::vector<vma::Allocation> color_alloc_{};
    std::vector<vk::ImageView> color_views_{};
    std::vector<vk::RenderingAttachmentInfo> color_infos_{};

    vk::Image depth_stencil_atchm_{};
    vma::Allocation depth_stencil_alloc_{};
    vk::ImageView depth_stencil_view_{};
    vk::RenderingAttachmentInfo depth_stencil_info_{};

    void operator()(fi::PipelineMgr& pipeline_mgr, fi::RenderMgr& render_mgr)
    {
        vk::PushConstantRange push_range{};
        push_range.size = 3 * sizeof(glm::mat4);
        push_range.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::PipelineLayoutCreateInfo p_layout_info{};
        auto set_layouts = render_mgr.texture_set_layouts();
        p_layout_info.setSetLayouts(set_layouts);
        p_layout_info.setPushConstantRanges(push_range);
        p_layout_ = pipeline_mgr.build_pipeline_layout(p_layout_info);

        input_asm_.topology = vk::PrimitiveTopology::eTriangleList;

        view_port_.viewportCount = 1;
        view_port_.scissorCount = 1;

        rasterizer_.lineWidth = 1;

        multi_sample_.rasterizationSamples = vk::SampleCountFlagBits::e1;

        ds_info_.depthTestEnable = true;
        ds_info_.depthCompareOp = vk::CompareOp::eLess;
        ds_info_.depthWriteEnable = true;

        hot_reload(pipeline_mgr);

        color_infos_.resize(3);
        for (int i = 0; i < 3; i++)
        {
            color_infos_[i].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            color_infos_[i].loadOp = vk::AttachmentLoadOp::eClear;
            color_infos_[i].clearValue = vk::ClearColorValue{0, 0, 0, 1};
            color_infos_[i].imageView = color_views_[i];
        }
        depth_stencil_info_.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        depth_stencil_info_.loadOp = vk::AttachmentLoadOp::eClear;
        depth_stencil_info_.clearValue = vk::ClearDepthStencilValue{{1.0f, 0}};
        depth_stencil_info_.imageView = depth_stencil_view_;
    }

    void hot_reload(fi::PipelineMgr& pipeline_mgr)
    {
        auto vtx_attributes = fi::Renderable::vtx_attributes();
        auto vtx_bindings = fi::Renderable::vtx_bindings();
        vtx_state_.setVertexAttributeDescriptions(vtx_attributes);
        vtx_state_.setVertexBindingDescriptions(vtx_bindings);

        std::vector<vk::PipelineColorBlendAttachmentState> color_state(3);
        for (int i = 0; i < 3; i++)
        {
            color_state[i].colorWriteMask = vk::ColorComponentFlagBits::eR | //
                                            vk::ColorComponentFlagBits::eG | //
                                            vk::ColorComponentFlagBits::eB | //
                                            vk::ColorComponentFlagBits::eA;
        }
        blend_states_.setAttachments(color_state);

        std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};
        dynamic_state_info_.setDynamicStates(dynamic_states);

        std::vector<vk::Format> formats = {vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Sfloat,
                                           vk::Format::eR32G32B32A32Sfloat};
        render_info_.setColorAttachmentFormats(formats);
        render_info_.setDepthAttachmentFormat(vk::Format::eD24UnormS8Uint);
        render_info_.setStencilAttachmentFormat(vk::Format::eD24UnormS8Uint);

        vs_.reset();
        fs_.reset();
        shader_infos_ = {vs_, fs_};

        vk::GraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.layout = p_layout_;
        pipeline_info.pNext = &render_info_;
        pipeline_info.setStages(shader_infos_);
        pipeline_info.pVertexInputState = &vtx_state_;
        pipeline_info.pInputAssemblyState = &input_asm_;
        pipeline_info.pTessellationState = &tesselation_info_;
        pipeline_info.pViewportState = &view_port_;
        pipeline_info.pRasterizationState = &rasterizer_;
        pipeline_info.pMultisampleState = &multi_sample_;
        pipeline_info.pDepthStencilState = &ds_info_;
        pipeline_info.pColorBlendState = &blend_states_;
        pipeline_info.pDynamicState = &dynamic_state_info_;
        pipeline_ = pipeline_mgr.build_pipeline(pipeline_info);
    }

    GBufferPass()
    {
        vk::ImageCreateInfo color_info{{},
                                       vk::ImageType::e2D,
                                       vk::Format::eR32G32B32A32Sfloat,
                                       {1920, 1080, 1},
                                       1,
                                       1,
                                       vk::SampleCountFlagBits::e1,
                                       vk::ImageTiling::eOptimal,
                                       vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                       {},
                                       {},
                                       {},
                                       {}};
        vma::AllocationCreateInfo color_alloc{{}, vma::MemoryUsage::eAutoPreferDevice};
        vk::ImageViewCreateInfo color_view{{},
                                           {},
                                           vk::ImageViewType::e2D,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           {},
                                           {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
        for (int i = 0; i < 3; i++)
        {
            auto result = allocator().createImage(color_info, color_alloc);
            color_atchms_.push_back(result.first);
            color_alloc_.push_back(result.second);
            color_view.image = result.first;
            color_views_.push_back(device().createImageView(color_view));
        }

        vk::ImageCreateInfo depth_info{{},
                                       vk::ImageType::e2D,
                                       vk::Format::eD24UnormS8Uint,
                                       {1920, 1080, 1},
                                       1,
                                       1,
                                       vk::SampleCountFlagBits::e1,
                                       vk::ImageTiling::eOptimal,
                                       vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                           vk::ImageUsageFlagBits::eSampled,
                                       {},
                                       {},
                                       {},
                                       {}};
        vma::AllocationCreateInfo depth_alloc{{}, vma::MemoryUsage::eAutoPreferDevice};
        vk::ImageViewCreateInfo depth_view{{},
                                           {},
                                           vk::ImageViewType::e2D,
                                           vk::Format::eD24UnormS8Uint, //
                                           {},
                                           {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}};
        auto result = allocator().createImage(depth_info, depth_alloc);
        depth_stencil_atchm_ = result.first;
        depth_stencil_alloc_ = result.second;
        depth_view.image = result.first;
        depth_stencil_view_ = device().createImageView(depth_view);

        vk::ImageMemoryBarrier barrier{};
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        vk::CommandBuffer cmd = one_time_submit_cmd();
        begin_cmd(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        for (int i = 0; i < 3; i++)
        {
            barrier.image = color_atchms_[i];
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                                {}, {}, {}, barrier);
        }
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        barrier.image = depth_stencil_atchm_;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);
        cmd.end();
        submit_one_time_cmd(cmd);
    }

    ~GBufferPass()
    {
        for (uint32_t i = 0; i < color_atchms_.size(); i++)
        {
            allocator().destroyImage(color_atchms_[i], color_alloc_[i]);
            device().destroyImageView(color_views_[i]);
        }
        allocator().destroyImage(depth_stencil_atchm_, depth_stencil_alloc_);
        device().destroyImageView(depth_stencil_view_);
        device().destroyPipelineLayout(p_layout_);
    }
};
