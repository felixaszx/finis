#include "pass.hpp"

Pass::Pass(const PassFunctions& funcs)
    : funcs_(funcs)
{
    images_.resize(funcs.image_count_);
    image_views_.resize(funcs.image_count_);
    atchm_infos_.resize(funcs.image_count_, vk::RenderingAttachmentInfo{});

    chain_info_.image_count_ = funcs.image_count_;
    chain_info_.images_ = (VkImage*)images_.data();
    chain_info_.image_views_ = (VkImageView*)image_views_.data();
    chain_info_.atchm_info_ = (VkRenderingAttachmentInfo*)atchm_infos_.data();

    funcs.init_(details_ptr(), &chain_info_);
}

Pass::~Pass()
{
    funcs_.clear_();
}

void Pass::setup()
{
    funcs_.setup_();
    for (uint32_t i = 0; i < images_.size(); i++)
    {
        atchm_infos_[i].imageView = image_views_[i];
        atchm_infos_[i].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    }
}

/**
 * @brief guarantee that when a render() is called, it must be a fresh start
 *
 * @param cmd
 */
void Pass::render(vk::CommandBuffer cmd)
{
    vk::RenderingInfo render_info{};
    render_info.renderArea = chain_info_.render_area_;
    render_info.colorAttachmentCount = chain_info_.image_count_ - chain_info_.depth_atchm_ - chain_info_.stencil_atchm_;
    render_info.pColorAttachments = atchm_infos_.data() + chain_info_.depth_atchm_ + chain_info_.stencil_atchm_;
    render_info.layerCount = chain_info_.layer_count_;
    if (chain_info_.depth_atchm_)
    {
        atchm_infos_[DEPTH_ATCHM_IDX].imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        render_info.pDepthAttachment = atchm_infos_.data();
    }
    if (chain_info_.stencil_atchm_)
    {
        atchm_infos_[STENCIL_ATCHM_IDX].imageLayout = vk::ImageLayout::eStencilAttachmentOptimal;
        render_info.pStencilAttachment = atchm_infos_.data() + 1;
    }

    cmd.beginRendering(render_info);
    funcs_.render_(cmd);
    cmd.endRendering();
}

void Pass::finish()
{
    funcs_.finish_();
}