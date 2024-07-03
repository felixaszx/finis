#include "graphics/directional_light.hpp"

fi::DirectionalLightData::DirectionalLightData(vk::Extent2D resolution, uint32_t input_atchm_count,
                                               uint32_t output_count)
    : map_extent_(resolution, 1)
{
    vk::ImageCreateInfo img_info({},                                        //
                                 vk::ImageType::e2D, vk::Format::eD16Unorm, //
                                 map_extent_, 1, 1,                         //
                                 vk::SampleCountFlagBits::e1,               //
                                 vk::ImageTiling::eOptimal,                 //
                                 vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                                 vk::SharingMode::eExclusive, //
                                 0, nullptr,                  //
                                 vk::ImageLayout::eUndefined);
    vma::AllocationCreateInfo img_alloc({}, vma::MemoryUsage::eAutoPreferDevice);
    auto result = allocator().createImage(img_info, img_alloc);
    shadow_map_ = result.first;
    map_alloc_ = result.second;

    vk::ImageViewCreateInfo view_info({},                                   //
                                      shadow_map_,                          //
                                      vk::ImageViewType::e2D, map_format(), //
                                      {},                                   //
                                      {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
    map_view_ = device().createImageView(view_info);

    atchm_info_ = {map_view_,
                   vk::ImageLayout::eDepthAttachmentOptimal,
                   {},
                   {},
                   {},
                   vk::AttachmentLoadOp::eClear,
                   vk::AttachmentStoreOp::eStore,
                   vk::ClearDepthStencilValue{1}};
    rendering_ = {{}, {{}, resolution}, 1, {}, 0, {}, &atchm_info_};

    vk::DescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = input_atchm_count;
    binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    vk::DescriptorSetLayoutCreateInfo set_layout_info{};
    set_layout_info.setBindings(binding);
    set_layout_ = device().createDescriptorSetLayout(set_layout_info);
}

fi::DirectionalLightData::~DirectionalLightData()
{
    device().destroyImageView(map_view_);
    allocator().destroyImage(shadow_map_, map_alloc_);
}

vk::ImageView fi::DirectionalLightData::shadow_map_view() const
{
    return map_view_;
}

vk::Format fi::DirectionalLightData::map_format() const
{
    return vk::Format::eD16Unorm;
}

void fi::DirectionalLightData::lock_and_load(vk::DescriptorPool des_pool)
{
}

vk::Extent3D fi::DirectionalLightData::map_extent() const
{
    return map_extent_;
}

void fi::DirectionalLightData::calculate_lighting(
                              const std::function<void(vk::DescriptorSet input_atchms_set, uint32_t input_atchm_binding,
                                                       uint32_t draw_vtx_cout)>& draw_func)
{
}
