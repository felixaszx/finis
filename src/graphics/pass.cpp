#include "graphics/pass.hpp"

Pass::Pass(const PassStates& states)
    : states_(states)
{
    images_.resize(states.image_count_);
    image_views_.resize(states.image_count_);
    atchm_infos_.resize(states.image_count_, vk::RenderingAttachmentInfo{});

    chain_info_.image_count_ = states.image_count_;
    chain_info_.images_ = (VkImage*)images_.data();
    chain_info_.image_views_ = (VkImageView*)image_views_.data();
    chain_info_.atchm_info_ = (VkRenderingAttachmentInfo*)atchm_infos_.data();

    states.init_(details_ptr(), &chain_info_);
}

Pass::~Pass()
{
    states_.clear_();
}

void Pass::setup()
{
    states_.setup_();
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
    states_.render_(cmd);
    cmd.endRendering();
}

void Pass::finish()
{
    states_.finish_();
}
Pass& PassGroup::register_pass(const PassStates& states)
{
    Pass& follow = passes_.emplace_back(states);
    follow.chain_info_.chain_id_ = passes_.size() - 1;
    if (passes_.size() > 1)
    {
        passes_[passes_.size() - 2].chain_info_.next_ = &follow.chain_info_;
        follow.chain_info_.prev_ = &passes_[passes_.size() - 2].chain_info_;
        follow.chain_info_.shared_info_ = &passes_[passes_.size() - 2].chain_info_.shared_info_;
    }
    return follow;
}

Pass* PassGroup::get_pass(uint32_t id)
{
    if (id < passes_.size())
    {
        return &passes_[id];
    }
    return nullptr;
}
