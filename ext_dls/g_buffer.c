#include "extensions/defines.h"

const ObjectDetails* details = NULL;
PassChain* this_pass = NULL;

enum ImageIdx
{
    DEPTH,
    POSITION,
    NORMAL,
    UV
};

VkImage* images = NULL;
VkImageView* image_views = NULL;
VmaAllocation image_alloc[4] = {};

void init_(const ObjectDetails* details_in, PassChain* this_pass_in)
{
    details = details_in;
    this_pass = this_pass_in;
    images = this_pass->images_;
    image_views = this_pass->image_views_;

    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.arrayLayers = 1;
    image_create_info.extent.width = 1920;
    image_create_info.extent.height = 1080;
    image_create_info.extent.depth = 1;
    image_create_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.mipLevels = 1;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    view_info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    for (int i = 1; i < 4; i++)
    {
        vmaCreateImage(details->allocator_, &image_create_info, &alloc_info, //
                       images + i, image_alloc + i, NULL);
        view_info.image = images[i];
        vkCreateImageView(details->device_, &view_info, NULL, image_views + i);
    }

    image_create_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    vmaCreateImage(details->allocator_, &image_create_info, &alloc_info, images + DEPTH, image_alloc + DEPTH, NULL);
    view_info.image = images[DEPTH];
    view_info.format = VK_FORMAT_D24_UNORM_S8_UINT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    vkCreateImageView(details->device_, &view_info, NULL, image_views + DEPTH);
}

void clear()
{
    for (int i = 0; i < 4; i++)
    {
        vkDestroyImageView(details->device_, image_views[i], NULL);
        vmaDestroyImage(details->allocator_, images[i], image_alloc[i]);
        images[i] = NULL;
        image_views[i] = NULL;
    }
}

void setup()
{
    this_pass->depth_atchm_ = true;
    this_pass->layer_count_ = 1;
    this_pass->render_area_.offset = (VkOffset2D){0, 0};
    this_pass->render_area_.extent = (VkExtent2D){1920, 1080};

    for (uint32_t i = 0; i < this_pass->image_count_; i++)
    {
        this_pass->atchm_info_[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        this_pass->atchm_info_[i].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        this_pass->atchm_info_[i].clearValue.color = (VkClearColorValue){0, 0, 0, 0};
    }
    this_pass->atchm_info_[DEPTH_ATCHM_IDX].clearValue.depthStencil = (VkClearDepthStencilValue){0, 0};
}

void render(VkCommandBuffer cmd)
{
}

void finish()
{
}

PassFunctions pass_func_getter()
{
    PassFunctions r;
    r.image_count_ = 4;
    r.init_ = init_;
    r.clear_ = clear;
    r.setup_ = setup;
    r.finish_ = finish;
    r.render_ = render;
    return r;
}