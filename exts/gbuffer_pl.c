#include "fi_ext.h"
#include "gfx/gfx.h"

FI_DLL_EXPORT VkPipelineLayout configurator(vk_ctx* ctx, vk_gfx_pl_desc* pl_desc)
{
    static VkFormat formats[4] = {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};
    pl_desc->atchms_.colorAttachmentCount = 4;
    pl_desc->atchms_.pColorAttachmentFormats = formats;
    pl_desc->atchms_.depthAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    pl_desc->atchms_.stencilAttachmentFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    pl_desc->input_asm_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pl_desc->viewport_.viewportCount = 1;
    pl_desc->viewport_.scissorCount = 1;
    pl_desc->rasterizer_.lineWidth = 1;
    pl_desc->rasterizer_.polygonMode = VK_POLYGON_MODE_FILL;
    pl_desc->multisample_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pl_desc->depth_stencil_.depthWriteEnable = true;
    pl_desc->depth_stencil_.depthTestEnable = true;
    pl_desc->depth_stencil_.depthCompareOp = VK_COMPARE_OP_LESS;

    static VkPipelineColorBlendAttachmentState blend_state[4] = {};
    for (size_t i = 0; i < 4; i++)
    {
        blend_state[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | //
                                        VK_COLOR_COMPONENT_G_BIT | //
                                        VK_COLOR_COMPONENT_B_BIT | //
                                        VK_COLOR_COMPONENT_A_BIT;
    }
    pl_desc->color_blend_.attachmentCount = 4;
    pl_desc->color_blend_.pAttachments = blend_state;

    static VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pl_desc->dynamic_state_.dynamicStateCount = 2;
    pl_desc->dynamic_state_.pDynamicStates = dynamic_states;

    VkPipelineLayout pl_layout = {};
    VkPipelineLayoutCreateInfo layout_cinfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_cinfo.pushConstantRangeCount = pl_desc->push_range_count_;
    layout_cinfo.pPushConstantRanges = pl_desc->push_range_;
    layout_cinfo.setLayoutCount = pl_desc->set_layout_count_;
    layout_cinfo.pSetLayouts = pl_desc->set_layouts_;
    vkCreatePipelineLayout(ctx->device_, &layout_cinfo, fi_nullptr, &pl_layout);

    construct_vk_shader(pl_desc->shaders_ + 0, ctx, "res/shaders/0.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    construct_vk_shader(pl_desc->shaders_ + 1, ctx, "res/shaders/0.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pl_desc->cinfo_.stageCount = 2;

    return pl_layout;
}

FI_DLL_EXPORT void cleaner(vk_ctx* ctx, vk_gfx_pl_desc* pl_desc)
{
    for (size_t i = 0; i < pl_desc->cinfo_.stageCount; i++)
    {
        destroy_vk_shader(pl_desc->shaders_ + i);
        memset(pl_desc->shaders_ + i, 0x0, sizeof(vk_shader));
    }
}