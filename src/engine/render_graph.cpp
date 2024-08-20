#include "engine/render_graph.hpp"

fi::RenderGraph::SyncedRes::Access fi::RenderGraph::SyncedRes::find_prev_access(PassIdx this_pass)
{
    auto lower_bound = std::lower_bound(history_.begin(), history_.end(), this_pass);
    if (lower_bound == history_.end())
    {
        return INITIAL;
    }
    return access_[lower_bound - history_.begin()];
}

fi::RenderGraph::SyncedRes::Access fi::RenderGraph::SyncedRes::find_next_access(PassIdx this_pass)
{
    auto upper_bound = std::upper_bound(history_.begin(), history_.end(), this_pass);
    if (upper_bound == history_.end())
    {
        return INITIAL;
    }
    return access_[upper_bound - history_.begin()];
}

fi::RenderGraph::Image& fi::RenderGraph::Pass::add_read_img(ResIdx img_idx,
                                                            const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = rg_->get_image_res(img_idx);
    input_.insert(img_idx);
    img.history_.push_back(idx_);
    img.access_.push_back(SyncedRes::READ);
    img.ranges_.push_back(sub_resources);
    return img;
}

fi::RenderGraph::Image& fi::RenderGraph::Pass::add_write_img(ResIdx img_idx,
                                                             const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = rg_->get_image_res(img_idx);
    output_.insert(img_idx);
    img.history_.push_back(idx_);
    img.access_.push_back(SyncedRes::WRITE);
    img.ranges_.push_back(sub_resources);
    return img;
}

void fi::RenderGraph::Pass::set_image_barrier(vk::ImageMemoryBarrier2& barrier,
                                              vk::ImageLayout old_layout,
                                              vk::ImageLayout new_layout)
{
    barrier.oldLayout = old_layout;
    switch (old_layout)
    {
    case vk::ImageLayout::eDepthReadOnlyOptimal:
    case vk::ImageLayout::eStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests;
        barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        break;
    case vk::ImageLayout::eDepthAttachmentOptimal:
    case vk::ImageLayout::eStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests;
        barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eGeneral:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
        barrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader //
                               | vk::PipelineStageFlagBits2::eComputeShader;
        barrier.srcAccessMask = vk::AccessFlagBits2::eShaderRead;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryRead;
        break;
    default:
        break;
    }

    barrier.newLayout = new_layout;
    switch (new_layout)
    {
    case vk::ImageLayout::eDepthReadOnlyOptimal:
    case vk::ImageLayout::eStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests;
        barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead;
        break;
    case vk::ImageLayout::eDepthAttachmentOptimal:
    case vk::ImageLayout::eStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eLateFragmentTests;
        barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eGeneral:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader //
                               | vk::PipelineStageFlagBits2::eComputeShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
        barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead;
        break;
    default:
        break;
    }
}

void fi::RenderGraph::Pass::pass_img(ResIdx img_idx,
                                     vk::ImageLayout taget_layout,
                                     const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = rg_->get_image_res(img_idx);
    passing_.insert(img_idx);
    img.history_.push_back(idx_);
    img.layouts_.push_back(taget_layout);
    img.access_.push_back(SyncedRes::PASS);
    img.ranges_.push_back(sub_resources);
}

void fi::RenderGraph::Pass::read_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = add_read_img(img_idx, sub_resources);
    img.layouts_.push_back(vk::ImageLayout::eShaderReadOnlyOptimal);
    img.usages_ |= vk::ImageUsageFlagBits::eStorage;
}

void fi::RenderGraph::Pass::sample_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = add_read_img(img_idx, sub_resources);
    img.layouts_.push_back(vk::ImageLayout::eShaderReadOnlyOptimal);
    img.usages_ |= vk::ImageUsageFlagBits::eSampled;
}

void fi::RenderGraph::ComputePass::write_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = add_write_img(img_idx, sub_resources);
    img.layouts_.push_back(vk::ImageLayout::eGeneral);
    img.usages_ |= vk::ImageUsageFlagBits::eStorage;
}

void fi::RenderGraph::GraphicsPass::clear_img(ResIdx res_idx)
{
    rg_->get_image_res(res_idx).cleared_pass_.push_back(idx_);
    clearing_.insert(res_idx);
}

void fi::RenderGraph::GraphicsPass::write_color_atchm(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources)
{
    Image& img = add_write_img(img_idx, sub_resources);
    img.layouts_.push_back(vk::ImageLayout::eColorAttachmentOptimal);
    img.usages_ |= vk::ImageUsageFlagBits::eColorAttachment;
}

void fi::RenderGraph::GraphicsPass::set_ds_atchm(ResIdx img_idx,           //
                                                 DepthStencilOp operation, //
                                                 const vk::ImageSubresourceRange& sub_resources)
{
    Image* img = nullptr;
    switch (operation)
    {
    // read only
    case DEPTH_READ:
    case STENCIL_READ:
    case DEPTH_READ | STENCIL_READ:
        img = &add_read_img(img_idx, sub_resources);
        break;
    // write
    case DEPTH_WRITE:
    case STENCIL_WRITE:
    case DEPTH_READ | STENCIL_WRITE:
    case DEPTH_WRITE | STENCIL_WRITE:
    case DEPTH_WRITE | STENCIL_READ:
        img = &add_write_img(img_idx, sub_resources);
        break;
    }
    img->usages_ |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

    switch (operation)
    {
    // read only
    case DEPTH_READ:
        img->layouts_.push_back(vk::ImageLayout::eDepthReadOnlyOptimal);
        break;
    case STENCIL_READ:
        img->layouts_.push_back(vk::ImageLayout::eStencilReadOnlyOptimal);
        break;
    case DEPTH_READ | STENCIL_READ:
        img->layouts_.push_back(vk::ImageLayout::eDepthStencilReadOnlyOptimal);
        break;
    // write
    case DEPTH_WRITE:
        img->layouts_.push_back(vk::ImageLayout::eDepthAttachmentOptimal);
        break;
    case STENCIL_WRITE:
        img->layouts_.push_back(vk::ImageLayout::eStencilAttachmentOptimal);
        break;
    case DEPTH_READ | STENCIL_WRITE:
        img->layouts_.push_back(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);
        break;
    case DEPTH_WRITE | STENCIL_WRITE:
        img->layouts_.push_back(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        break;
    case DEPTH_WRITE | STENCIL_READ:
        img->layouts_.push_back(vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal);
        break;
    }
}

fi::RenderGraph::~RenderGraph()
{
    for (Image& img : imgs_)
    {
        for (vk::ImageView view : img.views_)
        {
            device().destroyImageView(view);
        }
        if (img)
        {
            allocator().destroyImage(img, img);
        }
    }

    for (Buffer& buf : bufs_)
    {
        if (buf)
        {
            allocator().destroyBuffer(buf, buf);
        }
    }

    for (vk::Event event : events_)
    {
        device().destroyEvent(event);
    }
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_atchm(vk::ImageSubresource sub_res,
                                                        vk::Extent3D extent,
                                                        vk::Format format,
                                                        vk::ImageType type,
                                                        vk::ImageUsageFlagBits initial_usage)
{
    Image& img = imgs_.emplace_back();
    img.type_ = type;
    img.extent_ = extent;
    img.format_ = format;
    img.usages_ = initial_usage;
    img.idx_ = res_mapping_.size();
    img.sub_res_ = sub_res;
    res_mapping_.push_back(imgs_.size() - 1);
    res_types_.push_back(SyncedRes::IMAGE);
    return img.idx_;
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_atchm(vk::Image image, const std::vector<vk::ImageView>& views)
{
    Image& img = imgs_.emplace_back();
    sset(img, image);
    img.views_ = views;
    img.idx_ = res_mapping_.size();
    res_mapping_.push_back(imgs_.size() - 1);
    res_types_.push_back(SyncedRes::IMAGE);
    return img.idx_;
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_buffer(vk::DeviceSize size, bool presistant_mapping)
{
    Buffer& buf = bufs_.emplace_back();
    buf.idx_ = res_mapping_.size();
    buf.presistant_mapping_ = presistant_mapping;
    buf.range_ = size;
    res_mapping_.push_back(bufs_.size() - 1);
    res_types_.push_back(SyncedRes::BUFFER);
    return buf.idx_;
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_buffer(vk::Buffer buffer,
                                                         vk::DeviceSize offset,
                                                         vk::DeviceSize range,
                                                         bool presistant_mapping)
{
    Buffer& buf = bufs_.emplace_back();
    sset(buf, buffer);
    buf.offset_ = offset;
    buf.range_ = range;
    buf.idx_ = res_mapping_.size();
    res_mapping_.push_back(bufs_.size() - 1);
    res_types_.push_back(SyncedRes::BUFFER);
    return buf.idx_;
}

fi::RenderGraph::Buffer& fi::RenderGraph::get_buffer_res(ResIdx buf_idx)
{
    return bufs_[res_mapping_[buf_idx]];
};

fi::RenderGraph::Image& fi::RenderGraph::get_image_res(ResIdx img_idx)
{
    return imgs_[res_mapping_[img_idx]];
};

fi::RenderGraph::PassIdx fi::RenderGraph::register_graphics_pass(const GraphicsPass::Setup& setup_func)
{
    GraphicsPass& pass = graphics_.emplace_back();
    pass.rg_ = this;

    pass.idx_ = pass_mapping_.size();
    pass_mapping_.push_back(graphics_.size() - 1);
    pass_types_.push_back(Pass::GRAPHICS);
    if (setup_func)
    {
        setup_func(pass);
    }
    return pass.idx_;
}

fi::RenderGraph::PassIdx fi::RenderGraph::register_compute_pass(const ComputePass::Setup& setup_func)
{
    ComputePass& pass = computes_.emplace_back();
    pass.rg_ = this;

    pass.idx_ = pass_mapping_.size();
    pass_mapping_.push_back(computes_.size() - 1);
    pass_types_.push_back(Pass::COMPUTE);
    if (setup_func)
    {
        setup_func(pass);
    }
    return pass.idx_;
}

void fi::RenderGraph::set_compute_pass(PassIdx compute_pass_idx, const ComputePass::Setup& setup_func)
{
    ComputePass& pass = computes_[pass_mapping_[compute_pass_idx]];
    pass.rg_ = this;
    setup_func(pass);
}

void fi::RenderGraph::set_graphics_pass(PassIdx graphics_pass_idx, const GraphicsPass::Setup& setup_func)
{
    GraphicsPass& pass = graphics_[pass_mapping_[graphics_pass_idx]];
    pass.rg_ = this;
    setup_func(pass);
}

void fi::RenderGraph::allocate_res()
{
    // create nessary resources
    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    for (Image& img : imgs_)
    {
        if (casts(vk::Image, img) || img.layouts_.empty())
        {
            continue;
        }

        vk::ImageCreateInfo img_info{{},
                                     img.type_,
                                     img.format_,
                                     img.extent_,
                                     img.sub_res_.mipLevel,
                                     img.sub_res_.arrayLayer,
                                     vk::SampleCountFlagBits::e1,
                                     vk::ImageTiling::eOptimal,
                                     img.usages_,
                                     vk::SharingMode::eExclusive};
        vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
        vma::AllocationInfo alloc_result{};

        auto allocated = allocator().createImage(img_info, alloc_info, alloc_result);
        sset(img, allocated.first, allocated.second, alloc_result.deviceMemory);
        img.allocated_offset_ = alloc_result.offset;
        img.allocated_size_ = alloc_result.size;

        vk::ImageMemoryBarrier barrier{{},
                                       vk::AccessFlagBits::eTransferWrite,
                                       {},
                                       img.layouts_[0],
                                       VK_QUEUE_FAMILY_IGNORED,
                                       VK_QUEUE_FAMILY_IGNORED,
                                       img,
                                       ALL_IMAGE_SUBRESOURCES(img.sub_res_.aspectMask)};
        cmd.pipelineBarrier({}, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
    }
    cmd.end();
    submit_one_time_cmd(cmd);

    for (Buffer& buf : bufs_)
    {
        if (casts(vk::Buffer, buf))
        {
            continue;
        }

        vk::BufferCreateInfo buffer_info{{}, buf.range_, buf.usage_};
        vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
        vma::AllocationInfo alloc_result{};

        if (buf.presistant_mapping_)
        {
            alloc_info.preferredFlags |= vk::MemoryPropertyFlagBits::eHostCoherent;
        }
        auto allocated = allocator().createBuffer(buffer_info, alloc_info, alloc_result);
        sset(buf, allocated.first, allocated.second, alloc_result.deviceMemory);
        buf.allocated_offset_ = alloc_result.offset;
        buf.allocated_size_ = alloc_result.size;
    }

    for (auto p : pass_mapping_)
    {
        vk::EventCreateInfo event_info;
        event_info.flags = vk::EventCreateFlagBits::eDeviceOnly;
        events_.push_back(device().createEvent(event_info));
    }
}

void fi::RenderGraph::reset()
{
    for (Pass& pass : graphics_)
    {
        pass.wait_imgs_.clear();
        pass.input_.clear();
        pass.output_.clear();
        pass.passing_.clear();
        pass.clearing_.clear();
        pass.waiting_.clear();
    }
    for (Pass& pass : computes_)
    {
        pass.wait_imgs_.clear();
        pass.input_.clear();
        pass.output_.clear();
        pass.passing_.clear();
        pass.clearing_.clear();
        pass.waiting_.clear();
    }
    initial_transitions_.clear();
    final_transitions_.clear();

    for (Image& img : imgs_)
    {
        img.cleared_pass_.clear();
        img.layouts_.clear();
        img.ranges_.clear();
        img.access_.clear();
        img.history_.clear();
    }
}

void fi::RenderGraph::compile()
{
    for (Image& img : imgs_)
    {
        for (size_t h = 1; h < img.history_.size(); h++)
        {
            if (img.layouts_[h] == img.layouts_[h - 1] && //
                img.access_[h - 1] == SyncedRes::READ)
            {
                continue;
            }

            vk::ImageMemoryBarrier2* barrier = nullptr;
            if (pass_types_[img.history_[h]] == Pass::GRAPHICS)
            {
                barrier = &graphics_[pass_mapping_[img.history_[h]]].wait_imgs_.emplace_back();
                graphics_[pass_mapping_[img.history_[h]]].waiting_.insert(h - 1);
            }
            else
            {
                barrier = &computes_[pass_mapping_[img.history_[h]]].wait_imgs_.emplace_back();
                graphics_[pass_mapping_[img.history_[h]]].waiting_.insert(h - 1);
            }

            barrier->image = img;
            barrier->subresourceRange = img.ranges_[h];
            Pass::set_image_barrier(*barrier, img.layouts_[h - 1], img.layouts_[h]);
        }
    }

    for (Image& img : imgs_)
    {
        if (img.layouts_.back() != img.layouts_.front())
        {
            vk::ImageMemoryBarrier2* barrier = nullptr;
            if (img.access_.back() == SyncedRes::PASS)
            {
                barrier = &initial_transitions_.emplace_back();
            }
            else
            {
                barrier = &final_transitions_.emplace_back();
            }

            barrier->image = img;
            barrier->subresourceRange = ALL_IMAGE_SUBRESOURCES(img.sub_res_.aspectMask);
            Pass::set_image_barrier(*barrier, img.layouts_.back(), img.layouts_.front());
        }
    }

    for (Pass& pass : graphics_)
    {
        pass.dep_info_.setImageMemoryBarriers(pass.wait_imgs_);
    }
    for (Pass& pass : computes_)
    {
        pass.dep_info_.setImageMemoryBarriers(pass.wait_imgs_);
    }
}

void fi::RenderGraph::excute(vk::CommandBuffer cmd,
                             const std::vector<std::function<void(vk::CommandBuffer cmd)>>& funcs)
{
    vk::DependencyInfo init_dep;
    init_dep.setImageMemoryBarriers(initial_transitions_);
    cmd.pipelineBarrier2(init_dep);

    vk::Event prev_event = nullptr;
    const vk::DependencyInfo* prev_dep = nullptr;
    for (PassIdx p = 0; p < pass_mapping_.size(); p++)
    {
        if (prev_event)
        {
            cmd.waitEvents2(events_[p - 1], *prev_dep);
        }
        funcs[p](cmd);

        if (pass_types_[p] == Pass::GRAPHICS)
        {
            prev_dep = &graphics_[pass_mapping_[p]].dep_info_;
        }
        else
        {
            prev_dep = &computes_[pass_mapping_[p]].dep_info_;
        }
        prev_event = events_[p];
        cmd.setEvent2(prev_event, prev_dep);
    }
    cmd.waitEvents2(prev_event, *prev_dep);
}
