#include "engine/frame_graph.hpp"

bool fi::FrameImageRef::is_reading(vk::ImageLayout layout)
{
    switch (layout)
    {
    case vk::ImageLayout::eShaderReadOnlyOptimal:
    case vk::ImageLayout::eTransferSrcOptimal:
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthReadOnlyOptimal:
    case vk::ImageLayout::eStencilReadOnlyOptimal:
    case vk::ImageLayout::eReadOnlyOptimal:
    case vk::ImageLayout::ePresentSrcKHR:
        return true;
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthAttachmentOptimal:
    case vk::ImageLayout::eAttachmentOptimal:
    case vk::ImageLayout::eStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
    case vk::ImageLayout::eColorAttachmentOptimal:
    default:
        return false;
    }
}

vk::PipelineStageFlagBits2 fi::FrameImageRef::get_src_stage(vk::AccessFlagBits2 access)
{
    switch (access)
    {
    case vk::AccessFlagBits2::eColorAttachmentRead:
    case vk::AccessFlagBits2::eColorAttachmentWrite:
        return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    case vk::AccessFlagBits2::eDepthStencilAttachmentRead:
    case vk::AccessFlagBits2::eDepthStencilAttachmentWrite:
        return vk::PipelineStageFlagBits2::eLateFragmentTests;
    case vk::AccessFlagBits2::eTransferRead:
    case vk::AccessFlagBits2::eTransferWrite:
        return vk::PipelineStageFlagBits2::eAllTransfer;
    case vk::AccessFlagBits2::eShaderSampledRead:
        return vk::PipelineStageFlagBits2::eFragmentShader;
    case vk::AccessFlagBits2::eShaderStorageRead:
    case vk::AccessFlagBits2::eShaderStorageWrite:
        return vk::PipelineStageFlagBits2::eComputeShader;
    default:
        return vk::PipelineStageFlagBits2::eAllCommands;
    }
}

vk::PipelineStageFlagBits2 fi::FrameImageRef::get_dst_stage(vk::AccessFlagBits2 access)
{
    switch (access)
    {
    case vk::AccessFlagBits2::eColorAttachmentRead:
    case vk::AccessFlagBits2::eColorAttachmentWrite:
        return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    case vk::AccessFlagBits2::eDepthStencilAttachmentRead:
    case vk::AccessFlagBits2::eDepthStencilAttachmentWrite:
        return vk::PipelineStageFlagBits2::eEarlyFragmentTests;
    case vk::AccessFlagBits2::eTransferRead:
    case vk::AccessFlagBits2::eTransferWrite:
        return vk::PipelineStageFlagBits2::eAllTransfer;
    case vk::AccessFlagBits2::eShaderSampledRead:
        return vk::PipelineStageFlagBits2::eFragmentShader;
    case vk::AccessFlagBits2::eShaderStorageRead:
    case vk::AccessFlagBits2::eShaderStorageWrite:
        return vk::PipelineStageFlagBits2::eComputeShader;
    default:
        return vk::PipelineStageFlagBits2::eAllCommands;
    }
}

fi::FrameGraph::~FrameGraph()
{
    for (FrameImage& image : imgs_)
    {
        allocator().destroyImage(image, image);
    }

    for (FrameImageRef& ref : img_refs_)
    {
        device().destroyImageView(ref);
    }

    for (FramePass& pass : passes_)
    {
        for (const auto& event : pass.img_barriers_)
        {
            device().destroyEvent(event.second.first);
        }
    }
}

fi::FrameImage& fi::FrameGraph::register_image()
{
    return imgs_.emplace_back();
}

fi::ImgRefIdx fi::FrameGraph::register_image_ref(FrameImageRef& ref_detail)
{
    ref_detail = img_refs_.emplace_back();
    return img_refs_.size() - 1;
}

fi::PassIdx fi::FrameGraph::register_pass()
{
    FramePass& pass = passes_.emplace_back();
    return passes_.size() - 1;
}

void fi::FrameGraph::push_cluster_break()
{
    FramePass& pass = passes_.emplace_back();
}

void fi::FrameGraph::set_pass_func(PassIdx pass_idx, const PassFunc& func)
{
    passes_[pass_idx].func_ = func;
}

void fi::FrameGraph::read_image(PassIdx pass_idx, ImgRefIdx ref_idx)
{
    img_refs_[ref_idx].history_.emplace(pass_idx,                                          //
                                        std::pair(vk::ImageLayout::eShaderReadOnlyOptimal, //
                                                  vk::AccessFlagBits2::eShaderStorageRead));
}

void fi::FrameGraph::write_image(PassIdx pass_idx, ImgRefIdx ref_idx)
{
    img_refs_[ref_idx].history_.emplace(pass_idx,                            //
                                        std::pair(vk::ImageLayout::eGeneral, //
                                                  vk::AccessFlagBits2::eShaderStorageWrite));
}

void fi::FrameGraph::sample_image(PassIdx pass_idx, ImgRefIdx ref_idx)
{
    img_refs_[ref_idx].history_.emplace(pass_idx,                                          //
                                        std::pair(vk::ImageLayout::eShaderReadOnlyOptimal, //
                                                  vk::AccessFlagBits2::eShaderSampledRead));
}

void fi::FrameGraph::write_color_atchm(PassIdx pass_idx, ImgRefIdx ref_idx)
{
    img_refs_[ref_idx].history_.emplace(pass_idx,                                           //
                                        std::pair(vk::ImageLayout::eColorAttachmentOptimal, //
                                                  vk::AccessFlagBits2::eColorAttachmentWrite));
}

void fi::FrameGraph::set_depth_stencil(PassIdx pass_idx, ImgRefIdx ref_idx, DepthStencilOpMask operations)
{
    switch (operations)
    {
    case DEPTH_READ:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                         //
                                            std::pair(vk::ImageLayout::eDepthReadOnlyOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentRead));
        break;
    case DEPTH_WRITE:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                           //
                                            std::pair(vk::ImageLayout::eDepthAttachmentOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite));
        break;
    case STENCIL_READ:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                           //
                                            std::pair(vk::ImageLayout::eStencilReadOnlyOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentRead));
        break;
    case STENCIL_WRITE:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                             //
                                            std::pair(vk::ImageLayout::eStencilAttachmentOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite));
        break;
    case DEPTH_READ | STENCIL_READ:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                                //
                                            std::pair(vk::ImageLayout::eDepthStencilReadOnlyOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite));
        break;
    case DEPTH_READ | STENCIL_WRITE:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                                          //
                                            std::pair(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite));
        break;
    case DEPTH_WRITE | STENCIL_READ:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                                          //
                                            std::pair(vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite));
        break;
    case DEPTH_WRITE | STENCIL_WRITE:
        img_refs_[ref_idx].history_.emplace(pass_idx,                                                  //
                                            std::pair(vk::ImageLayout::eDepthStencilAttachmentOptimal, //
                                                      vk::AccessFlagBits2::eDepthStencilAttachmentWrite));
        break;
    default:
        break;
    }
}

void fi::FrameGraph::reset()
{
    if (!loaded)
    {
        loaded = true;
        vk::CommandBuffer cmd = one_time_submit_cmd();
        begin_cmd(cmd);
        for (FrameImage& image : imgs_)
        {
            vk::ImageCreateInfo img_info{{},
                                         image.type_,
                                         image.format_,
                                         image.extent_,
                                         image.sub_resources_.mipLevel,
                                         image.sub_resources_.arrayLayer,
                                         vk::SampleCountFlagBits::e1,
                                         vk::ImageTiling::eOptimal,
                                         image.usage_};

            vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};

            auto allocated = allocator().createImage(img_info, alloc_info);
            sset(image, allocated.first, allocated.second);

            vk::ImageMemoryBarrier barrier{
                {},
                vk::AccessFlagBits::eTransferWrite,
                {},
                image.init_layout_,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                image,
                {image.sub_resources_.aspectMask, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS}};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost,     //
                                vk::PipelineStageFlagBits::eTransfer, //
                                {}, {}, {}, barrier);
        }
        cmd.end();
        submit_one_time_cmd(cmd);
    }

    for (FrameImageRef& ref : img_refs_)
    {
        ref.history_.clear();
        if (!ref)
        {
            vk::ImageViewCreateInfo view_info{{}, ref.image_, ref.type_, ref.format_, {}, ref.range_};
            sset(ref, device().createImageView(view_info));
        }
    }

    for (FramePass& pass : passes_)
    {
        pass.img_barriers_.clear();
    }
}

void fi::FrameGraph::compile()
{
    for (FrameImageRef& ref : img_refs_)
    {
        auto record = ref.history_.begin();
        auto prev_record = record;
        record++;
        while (record != ref.history_.end())
        {
        }
    }
}