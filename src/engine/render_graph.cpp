#include "engine/render_graph.hpp"

fi::RenderGraph::ResIdx fi::RenderGraph::register_atchm(vk::ImageSubresource sub_res,
                                                        vk::ImageUsageFlagBits initial_usage)
{
    Image& img = imgs_.emplace_back();
    img.usages_ = initial_usage;
    img.idx_ = res_mapping_.size();
    img.sub_res_ = sub_res;
    res_mapping_.push_back(imgs_.size() - 1);
    return img.idx_;
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_atchm(vk::Image image, const std::vector<vk::ImageView>& views)
{
    Image& img = imgs_.emplace_back();
    sset(img, image);
    img.views_ = views;
    img.idx_ = res_mapping_.size();
    res_mapping_.push_back(imgs_.size() - 1);
    return img.idx_;
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_buffer(vk::DeviceSize size)
{
    Buffer& buf = bufs_.emplace_back();
    buf.idx_ = res_mapping_.size();
    buf.range_ = size;
    res_mapping_.push_back(bufs_.size() - 1);
    return buf.idx_;
}

fi::RenderGraph::ResIdx fi::RenderGraph::register_buffer(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range)
{
    Buffer& buf = bufs_.emplace_back();
    sset(buf, buffer);
    buf.offset_ = offset;
    buf.range_ = range;

    buf.idx_ = res_mapping_.size();
    res_mapping_.push_back(bufs_.size() - 1);
    return buf.idx_;
}

fi::RenderGraph::PassIdx fi::RenderGraph::register_graphics_pass(
    const std::function<void(GraphicsPass& pass)>& setup_func)
{
    GraphicsPass& pass = graphics_.emplace_back();
    pass.rg_ = this;

    pass.idx_ = pass_mapping_.size();
    pass_mapping_.push_back(graphics_.size() - 1);
    setup_func(pass);
    return pass.idx_;
}

fi::RenderGraph::PassIdx fi::RenderGraph::register_compute_pass(
    const std::function<void(ComputePass& pass)>& setup_func)
{
    ComputePass& pass = computes_.emplace_back();
    pass.rg_ = this;

    pass.idx_ = pass_mapping_.size();
    pass_mapping_.push_back(computes_.size() - 1);
    setup_func(pass);
    return pass.idx_;
}

fi::RenderGraph::ComputePass& fi::RenderGraph::get_compute_pass(PassIdx compute_pass_idx)
{
    return computes_[pass_mapping_[compute_pass_idx]];
}

fi::RenderGraph::GraphicsPass& fi::RenderGraph::get_graphics_pass(PassIdx graphics_pass_idx)
{
    return graphics_[pass_mapping_[graphics_pass_idx]];
}

fi::RenderGraph::Buffer& fi::RenderGraph::get_buffer_res(ResIdx buf_idx)
{
    return bufs_[res_mapping_[buf_idx]];
};

fi::RenderGraph::Image& fi::RenderGraph::get_image_res(ResIdx img_idx)
{
    return imgs_[res_mapping_[img_idx]];
};

fi::RenderGraph::Image& fi::RenderGraph::Pass::add_read_img(ResIdx img_idx,
                                                            const vk::ImageSubresourceRange& sub_resources)
{
    input_.insert(img_idx);
    Image& img = rg_->get_image_res(img_idx);
    img.history_.push_back(idx_);
    img.access.push_back(SyncedRes::READ);
    img.ranges_.push_back(sub_resources);
    return img;
}

fi::RenderGraph::Image& fi::RenderGraph::Pass::add_write_img(ResIdx img_idx,
                                                             const vk::ImageSubresourceRange& sub_resources)
{
    output_.insert(img_idx);
    Image& img = rg_->get_image_res(img_idx);
    img.history_.push_back(idx_);
    img.access.push_back(SyncedRes::WRITE);
    img.ranges_.push_back(sub_resources);
    return img;
}

void fi::RenderGraph::Pass::read_buffer(ResIdx buf_idx)
{
    input_.insert(buf_idx);
    Buffer& buf = rg_->get_buffer_res(buf_idx);
    buf.history_.push_back(idx_);
    buf.access.push_back(SyncedRes::READ);
}

void fi::RenderGraph::Pass::write_buffer(ResIdx buf_idx)
{
    output_.insert(buf_idx);
    Buffer& buf = rg_->get_buffer_res(buf_idx);
    buf.history_.push_back(idx_);
    buf.access.push_back(SyncedRes::WRITE);
}

void fi::RenderGraph::Pass::pass_img(ResIdx img_idx, vk::ImageLayout taget_layout,
                                     const vk::ImageSubresourceRange& sub_resources)
{
    passing_.insert(img_idx);
    Image& img = rg_->get_image_res(img_idx);
    img.history_.push_back(idx_);
    img.layouts_.push_back(taget_layout);
    img.access.push_back(SyncedRes::PASS);
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
