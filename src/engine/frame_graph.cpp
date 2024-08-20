#include "engine/frame_graph.hpp"

bool fi::FrameImageRef::is_writing(vk::ImageLayout layout)
{
    switch (layout)
    {
    case vk::ImageLayout::eShaderReadOnlyOptimal:
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eDepthReadOnlyOptimal:
    case vk::ImageLayout::eStencilReadOnlyOptimal:
    case vk::ImageLayout::eReadOnlyOptimal:
    case vk::ImageLayout::ePresentSrcKHR:
        return false;
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::eTransferSrcOptimal:
    case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
    case vk::ImageLayout::eDepthAttachmentOptimal:
    case vk::ImageLayout::eAttachmentOptimal:
    case vk::ImageLayout::eStencilAttachmentOptimal:
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
    case vk::ImageLayout::eColorAttachmentOptimal:
    default:
        return true;
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
    passes_.emplace_back();
    return passes_.size() - 1;
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

void fi::FrameGraph::pass_image(PassIdx pass_idx, ImgRefIdx ref_idx, vk::ImageLayout layout)
{
    img_refs_[ref_idx].history_.emplace(pass_idx, std::pair(layout, vk::AccessFlagBits2::eMemoryRead));
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
    }

    for (FrameImageRef& ref : img_refs_)
    {
        ref.history_.clear();
    }
}