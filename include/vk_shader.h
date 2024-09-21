#ifndef INCLUDE_VK_SHADER_H
#define INCLUDE_VK_SHADER_H

#include <spirv_cross_c.h>

#include "fi_vk.h"

typedef struct vk_shader_desc_set
{
    uint32_t binding_limit_;
    uint32_t binding_size_;
    VkDescriptorSetLayoutBinding* bindings_;
    char (*desc_name_)[64];
} vk_shader_desc_set;

typedef struct vk_shader
{
    uint32_t atchm_size_;
    VkRenderingAttachmentInfo* atchms_;
    VkPushConstantRange push_const_;

    uint32_t set_limit_;
    uint32_t set_size_;
    vk_shader_desc_set* desc_sets_;

    vk_ctx* ctx_;
    VkShaderModule module_;
    VkPipelineShaderStageCreateInfo stage_info_;
} vk_shader;

DEFINE_OBJ(vk_shader, vk_ctx* ctx, const char* file_path, VkShaderStageFlags stage);
void vk_shader_reflect_symbols(vk_shader* this);

enum vk_pl_stage
{
    GFX_PIPELINE_STAGE_VERT,
    GFX_PIPELINE_STAGE_FRAG,
    GFX_PIPELINE_STAGE_GEOM,
    GFX_PIPELINE_STAGE_COMP = 0,
};

typedef struct vk_pl
{
    VkPipeline pipeline_;
    VkPipelineLayout layout_;
} vk_pl;

typedef struct vk_gfx_pl_desc
{
    vk_pl pl_;
    VkGraphicsPipelineCreateInfo cinfo_;
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

DEFINE_OBJ_DEFAULT(vk_gfx_pl);

typedef struct vk_comp_pl
{
    vk_pl pl_;
    VkComputePipelineCreateInfo cinfo_;
    VkPipelineShaderStageCreateInfo stage_;
} vk_comp_pl;

DEFINE_OBJ_DEFAULT(vk_comp_pl);

#endif // INCLUDE_VK_SHADER_H