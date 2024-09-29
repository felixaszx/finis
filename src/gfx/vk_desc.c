#include "vk_desc.h"

VkResult vk_desc_set_base_create_layout(vk_desc_set_base* this, vk_ctx* ctx)
{
    VkDescriptorBindingFlags flags[this->binding_count_];
    for (size_t i = 0; i < this->binding_count_; i++)
    {
        flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    }
    flags[this->binding_count_ - 1] |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info //
        = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    flag_info.bindingCount = this->binding_count_;
    flag_info.pBindingFlags = flags;

    VkDescriptorSetLayoutCreateInfo set_layout_info_ = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_layout_info_.pNext = &flag_info;
    set_layout_info_.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    set_layout_info_.bindingCount = this->binding_count_;
    set_layout_info_.pBindings = this->bindings_;

    VkResult result = vkCreateDescriptorSetLayout(ctx->device_, &set_layout_info_, nullptr, &this->layout_);
    return result;
}

VkDescriptorSet vk_desc_set_base_alloc_set(vk_desc_set_base* this,
                                           vk_ctx* ctx,
                                           VkDescriptorPool pool,
                                           uint32_t desc_count)
{
    VkDescriptorSetVariableDescriptorCountAllocateInfo count_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    count_info.descriptorSetCount = 1;
    count_info.pDescriptorCounts = &desc_count;

    VkDescriptorSetAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.pNext = &count_info;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &this->layout_;

    VkDescriptorSet set = {};
    vkAllocateDescriptorSets(ctx->device_, &alloc_info, &set);
    return set;
}

void vk_desc_pool_add_desc_count(vk_desc_pool* this, VkDescriptorType type, uint32_t size)
{
    this->sizes_[this->size_count_].type = type;
    this->sizes_[this->size_count_].descriptorCount = size;
    this->size_count_++;
}

VkResult vk_desc_pool_create(vk_desc_pool* this, vk_ctx* ctx, uint32_t set_limit)
{
    VkDescriptorPoolCreateInfo cinfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    cinfo.poolSizeCount = this->size_count_;
    cinfo.pPoolSizes = this->sizes_;
    cinfo.maxSets = set_limit;
    cinfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    return vkCreateDescriptorPool(ctx->device_, &cinfo, nullptr, &this->pool_);
}