#ifndef INCLUDE_VK_DESC_H
#define INCLUDE_VK_DESC_H

#include "fi_vk.h"

// only variable size will be used in this system
// only 1 binding will be used in this system
typedef struct vk_desc_set
{
    uint32_t limit_;
    VkDescriptorType type_;
    VkDescriptorSet set_;          // free externally
    VkDescriptorSetLayout layout_; // free externally
} vk_desc_set;

VkResult vk_desc_set_allocate(vk_desc_set* this, vk_ctx* ctx, VkDescriptorPool pool);
VkResult vk_desc_set_free_set(vk_desc_set* this, vk_ctx* ctx, VkDescriptorPool pool);
void vk_desc_set_free_layout(vk_desc_set* this, vk_ctx* ctx);

#endif // INCLUDE_VK_DESC_H