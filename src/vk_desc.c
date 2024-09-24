#include "vk_desc.h"

VkResult vk_desc_set_allocate(vk_desc_set* this, vk_ctx* ctx, VkDescriptorPool pool)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.stageFlags = VK_SHADER_STAGE_ALL;
    binding.descriptorType = this->type_;

    VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info //
        = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    static const VkDescriptorBindingFlagsEXT flags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                                                     VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                                     VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    flag_info.bindingCount = 1;
    flag_info.pBindingFlags = &flags;

    VkDescriptorSetLayoutCreateInfo set_layout_info_ = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_layout_info_.pNext = &flag_info;
    set_layout_info_.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    set_layout_info_.bindingCount = 1;
    set_layout_info_.pBindings = &binding;
    vkCreateDescriptorSetLayout(ctx->device_, &set_layout_info_, nullptr, &this->layout_);

    VkDescriptorSetVariableDescriptorCountAllocateInfo count_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    count_info.descriptorSetCount = 1;
    count_info.pDescriptorCounts = &this->limit_;

    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.pNext = &count_info;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &this->layout_;

    return vkAllocateDescriptorSets(ctx->device_, &alloc_info, &this->set_);
}

void vk_desc_set_free_layout(vk_desc_set* this, vk_ctx* ctx)
{
    vkDestroyDescriptorSetLayout(ctx->device_, this->layout_, nullptr);
    this->layout_ = nullptr;
}

VkResult vk_desc_set_free(vk_desc_set* this, vk_ctx* ctx, VkDescriptorPool pool)
{
    VkDescriptorSet set = this->set_;
    this->set_ = nullptr;
    return vkFreeDescriptorSets(ctx->device_, pool, 1, &set);
}
