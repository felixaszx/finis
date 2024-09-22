#ifndef INCLUDE_VK_PIPELINE_H
#define INCLUDE_VK_PIPELINE_H

#include "fi_vk.h"

typedef struct vk_shader
{
    vk_ctx* ctx_;
    VkShaderModule module_;
    VkPipelineShaderStageCreateInfo stage_info_;
} vk_shader;

DEFINE_OBJ(vk_shader, vk_ctx* ctx, const char* file_path, VkShaderStageFlags stage);
void vk_shader_reflect_symbols(vk_shader* this);

typedef struct vk_gfx_pl_desc vk_gfx_pl_desc;
typedef VkPipelineLayout (*vk_gfx_pl_configurator)(vk_ctx* ctx, vk_gfx_pl_desc* pl_desc);
typedef void (*vk_gfx_pl_cleaner)(vk_ctx* ctx, vk_gfx_pl_desc* pl_desc);

struct vk_gfx_pl_desc
{
    vk_shader shaders_[3];
    VkGraphicsPipelineCreateInfo cinfo_;
    VkPipelineShaderStageCreateInfo stages_[3]; // no more than 3 stages for now
    VkPipelineRenderingCreateInfo atchms_;
    VkPipelineVertexInputStateCreateInfo vtx_input_;
    VkPipelineInputAssemblyStateCreateInfo input_asm_;
    VkPipelineTessellationStateCreateInfo tessellation_;
    VkPipelineViewportStateCreateInfo viewport_;
    VkPipelineRasterizationStateCreateInfo rasterizer_;
    VkPipelineMultisampleStateCreateInfo multisample_;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_;
    VkPipelineColorBlendStateCreateInfo color_blend_;
    VkPipelineDynamicStateCreateInfo dynamic_state_;

    vk_gfx_pl_configurator configurator_;
    vk_gfx_pl_cleaner cleaner_;
};

DEFINE_OBJ(vk_gfx_pl_desc, vk_gfx_pl_configurator configurator, vk_gfx_pl_cleaner cleaner);
VkPipeline vk_gfx_pl_desc_build(vk_gfx_pl_desc* this, vk_ctx* ctx, VkPipelineLayout* layout);

typedef struct vk_comp_pl_desc vk_comp_pl_desc;
typedef VkPipelineLayout (*vk_comp_pl_configurator)(vk_ctx* ctx, vk_comp_pl_desc* pl_desc);
typedef void (*vk_comp_pl_cleaner)(vk_ctx* ctx, vk_comp_pl_desc* pl_desc);

struct vk_comp_pl_desc
{
    vk_shader shader_[1]; // only 1
    VkComputePipelineCreateInfo cinfo_;

    vk_comp_pl_configurator configurator_;
    vk_comp_pl_cleaner cleaner_;
};

DEFINE_OBJ(vk_comp_pl_desc, vk_comp_pl_configurator configurator, vk_comp_pl_cleaner cleaner);
VkPipeline vk_comp_pl_desc_build(vk_comp_pl_desc* this, vk_ctx* ctx, VkPipelineLayout* layout);

#endif // INCLUDE_VK_PIPELINE_H