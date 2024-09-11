#include "graphics/pipeline.hpp"

void fi::gfx::gfx_pipeline::config()
{
    vk::PipelineLayoutCreateInfo pl_layout_info{};
    std::vector<vk::PushConstantRange> ranges;
    ranges.reserve(shader_ref_->push_consts_.size());
    for (uint32_t r = 0; r < shader_ref_->push_consts_.size(); r++)
    {
        ranges.emplace_back(vk::PushConstantRange(shader_ref_->push_stages_[r], //
                                                  0,                       //
                                                  shader_ref_->push_consts_[r].second));
    }
    pl_layout_info.setSetLayouts(set_layouts_);
    pl_layout_info.setPushConstantRanges(ranges);
    layout_ = device().createPipelineLayout(pl_layout_info);
}
