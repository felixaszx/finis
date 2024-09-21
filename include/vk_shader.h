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

enum vk_pl_stage
{
    GFX_PIPELINE_STAGE_VERT,
    GFX_PIPELINE_STAGE_FRAG,
    GFX_PIPELINE_STAGE_GEOM,
    GFX_PIPELINE_STAGE_COMP = 0,
};

typedef struct vk_pl
{
    VkPipeline pl_;
    VkPipelineLayout layout_;
} vk_pl;

typedef struct vk_gfx_pl
{
    vk_pl pl_;
    VkPipelineShaderStageCreateInfo stages_[3];
    VkPipelineRenderingCreateInfo atchms_;
    VkPipelineVertexInputStateCreateInfo vtx_input_;
    VkPipelineInputAssemblyStateCreateInfo input_asm_;
    VkPipelineTessellationStateCreateInfo tessellation_;
    VkPipelineViewportStateCreateInfo viewport_;
    VkPipelineRasterizationStateCreateInfo rasterizer_;
    VkPipelineMultisampleStateCreateInfo multi_sample_;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_;
    VkPipelineColorBlendStateCreateInfo color_blend_;
    VkPipelineDynamicStateCreateInfo dynamic_state_;
} vk_gfx_pl;

typedef struct vk_comp_pl
{
    vk_pl pl_;
    VkPipelineShaderStageCreateInfo stages_[1];
} vk_comp_pl;

#endif // INCLUDE_VK_SHADER_H