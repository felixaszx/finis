#include <iostream>

#include "extensions/cpp_defines.hpp"
#include "engine/pipeline.hpp"
#include "tools.hpp"

std::vector<vk::Image> images_{};
std::vector<vma::Allocation> allocations_{};
std::vector<vk::ImageView> views_{};

struct Pipeline0 : public fi::GraphicsPipelineBase
{
    std::vector<vk::Format> formats_;

    void get_pipeline_info(uint32_t width, uint32_t height) override
    {
        using namespace fi;
        vk::ImageCreateInfo atchm_info{{},
                                       vk::ImageType::e2D,
                                       vk::Format::eR32G32B32A32Sfloat,
                                       {width, height, 1},
                                       1,
                                       1,
                                       vk::SampleCountFlagBits::e1,
                                       vk::ImageTiling::eOptimal,
                                       vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
                                       vk::SharingMode::eExclusive};
        vma::AllocationCreateInfo atchm_alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
        vk::ImageViewCreateInfo atchm_view_info{{}, //
                                                {}, //
                                                vk::ImageViewType::e2D,
                                                atchm_info.format,
                                                {},
                                                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
        images_.resize(5);
        views_.resize(5);
        allocations_.resize(5);

        for (uint32_t i = 0; i < 4; i++)
        {
            auto allocated = allocator().createImage(atchm_info, atchm_alloc_info);
            images_[i] = allocated.first;
            allocations_[i] = allocated.second;
            atchm_view_info.image = allocated.first;
            views_[i] = device().createImageView(atchm_view_info);
        }

        vk::ImageCreateInfo ds_info{{},
                                    vk::ImageType::e2D,
                                    vk::Format::eD24UnormS8Uint,
                                    {width, height, 1},
                                    1,
                                    1,
                                    vk::SampleCountFlagBits::e1,
                                    vk::ImageTiling::eOptimal,
                                    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                    vk::SharingMode::eExclusive};
        vk::ImageViewCreateInfo ds_view_info{
            {},
            {},
            vk::ImageViewType::e2D,
            vk::Format::eD24UnormS8Uint,
            {},
            {vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1}};
        auto ds_allocated = allocator().createImage(ds_info, atchm_alloc_info);
        images_[4] = ds_allocated.first;
        allocations_[4] = ds_allocated.second;
        ds_view_info.image = ds_allocated.first;
        views_[4] = device().createImageView(ds_view_info);

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
        for (int i = 0; i < 4; i++)
        {
            barrier.image = images_[i];
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                                {}, {}, {}, barrier);
        }
        barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        barrier.image = images_[4];
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);
        cmd.end();
        submit_one_time_cmd(cmd);

        atchm_infos_.resize(5);
        for (int i = 0; i < 4; i++)
        {
            atchm_infos_[i].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            atchm_infos_[i].loadOp = vk::AttachmentLoadOp::eClear;
            atchm_infos_[i].clearValue = vk::ClearColorValue{0, 0, 0, 1};
            atchm_infos_[i].imageView = views_[i];
        }
        atchm_infos_[4].imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        atchm_infos_[4].loadOp = vk::AttachmentLoadOp::eClear;
        atchm_infos_[4].clearValue = vk::ClearDepthStencilValue{{1.0f, 0}};
        atchm_infos_[4].imageView = views_[4];

        // set pso
        vs_.reset("res/shaders/vulkan0.vert", vk::ShaderStageFlagBits::eVertex);
        fs_.reset("res/shaders/vulkan0.frag", vk::ShaderStageFlagBits::eFragment);
        shader_stages_ = {vs_, fs_};

        input_asm_.topology = vk::PrimitiveTopology::eTriangleList;
        view_port_.viewportCount = 1;
        view_port_.scissorCount = 1;
        rasterizer_.lineWidth = 1;
        multi_sample_.rasterizationSamples = vk::SampleCountFlagBits::e1;
        pso_ds_info_.depthTestEnable = true;
        pso_ds_info_.depthCompareOp = vk::CompareOp::eLess;
        pso_ds_info_.depthWriteEnable = true;

        atchm_blends_.resize(4);
        for (auto& c : atchm_blends_)
        {
            c.colorWriteMask = vk::ColorComponentFlagBits::eR | //
                               vk::ColorComponentFlagBits::eG | //
                               vk::ColorComponentFlagBits::eB | //
                               vk::ColorComponentFlagBits::eA;
        }
        blend_states_.setAttachments(atchm_blends_);

        dynamic_states_ = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};
        dynamic_state_info_.setDynamicStates(dynamic_states_);

        formats_ = {atchm_info.format, atchm_info.format, atchm_info.format, atchm_info.format};
        pipeline_rendering_.setColorAttachmentFormats(formats_);
        pipeline_rendering_.setDepthAttachmentFormat(ds_info.format);
        pipeline_rendering_.setStencilAttachmentFormat(ds_info.format);

        vk::GraphicsPipelineCreateInfo pso_info{};
        pso_info.pNext = &pipeline_rendering_;
        pso_info.setStages(shader_stages_);
        pso_info.pInputAssemblyState = &input_asm_;
        pso_info.pTessellationState = &tesselation_info_;
        pso_info.pViewportState = &view_port_;
        pso_info.pRasterizationState = &rasterizer_;
        pso_info.pMultisampleState = &multi_sample_;
        pso_info.pDepthStencilState = &pso_ds_info_;
        pso_info.pColorBlendState = &blend_states_;
        pso_info.pDynamicState = &dynamic_state_info_;

        rendering_info_.pColorAttachments = atchm_infos_.data();
        rendering_info_.colorAttachmentCount = 4;
        rendering_info_.pDepthAttachment = &atchm_infos_[4];
        rendering_info_.layerCount = 1;
        rendering_info_.renderArea = {{}, width, height};
    }

    std::vector<vk::Image>& get_images() override { return images_; }
    std::vector<vk::ImageView>& get_image_views() override { return views_; }

    std::vector<uint32_t> get_color_atchm_idx() override { return {0, 1, 2, 3}; }
    uint32_t get_depth_atchm_idx() override { return 4; }

    ~Pipeline0()
    { // free resources
        for (size_t i = 0; i < images_.size(); i++)
        {
            device().destroyImageView(views_[i]);
            allocator().destroyImage(images_[i], allocations_[i]);
        }
    }
};

EXPORT_EXTENSION(Pipeline0);