#ifndef INCLUDE_VK_SHADER_H
#define INCLUDE_VK_SHADER_H

#include "fi_vk.h"

typedef struct vk_shader_layout
{
    uint32_t set_size_;
    uint32_t* binding_size_;
} vk_shader_layout;

typedef struct vk_shader
{
    vk_shader_layout layout_;

    vk_ctx* ctx_;
    VkShaderModule module_;
    VkPipelineShaderStageCreateInfo stage_info_;
} vk_shader;

DEFINE_OBJ(vk_shader, vk_ctx* ctx, const char* file_path, VkShaderStageFlags stage);
DEFINE_OBJ_DELETE(vk_shader);

#endif // INCLUDE_VK_SHADER_H