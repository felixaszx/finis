#ifndef INCLUDE_VK_DESC_H
#define INCLUDE_VK_DESC_H

#include "fi_vk.h"

#define DESC_POOL_SIZES_COUNT 11
typedef struct vk_desc_pool
{
    // use VkDescriptorType
    uint32_t size_count_;
    VkDescriptorPoolSize sizes_[DESC_POOL_SIZES_COUNT];
    VkDescriptorPool pool_; // free externally
} vk_desc_pool;

void vk_desc_pool_add_desc_count(vk_desc_pool* this, VkDescriptorType type, uint32_t size);
VkResult vk_desc_pool_create(vk_desc_pool* this, vk_ctx* ctx, uint32_t set_limit);

// only variable size will be used in this system
// only 1 binding will be used in this system
// so each set will only have 1 type of dewcriptor
typedef struct vk_desc_set_base
{
    uint32_t limit_;
    VkDescriptorType type_;
    VkDescriptorSetLayout layout_; // free externally
} vk_desc_set_base;

VkResult vk_desc_set_base_create_layout(vk_desc_set_base* this, vk_ctx* ctx);
VkDescriptorSet vk_desc_set_base_alloc_set(vk_desc_set_base* this,
                                           vk_ctx* ctx,
                                           VkDescriptorPool pool,
                                           uint32_t desc_count);

#endif // INCLUDE_VK_DESC_H