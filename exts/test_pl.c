#include "fi_ext.h"
#include "vk_pipeline.h"

DLL_EXPORT VkPipelineLayout configurator(vk_ctx* ctx, vk_gfx_pl_desc* pl_desc)
{
    static VkFormat color_format[1] = {VK_FORMAT_R32G32B32A32_SFLOAT};
    pl_desc->atchms_.colorAttachmentCount = 1;
    pl_desc->atchms_.pColorAttachmentFormats = color_format;
    pl_desc->input_asm_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pl_desc->viewport_.viewportCount = 1;
    pl_desc->viewport_.scissorCount = 1;
    pl_desc->rasterizer_.lineWidth = 1;
    pl_desc->rasterizer_.polygonMode = VK_POLYGON_MODE_FILL;
    pl_desc->multisample_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pl_desc->depth_stencil_.depthWriteEnable = false;

    static VkPipelineColorBlendAttachmentState blend_state[1] = {};
    blend_state[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | //
                                    VK_COLOR_COMPONENT_G_BIT | //
                                    VK_COLOR_COMPONENT_B_BIT | //
                                    VK_COLOR_COMPONENT_A_BIT;
    pl_desc->color_blend_.attachmentCount = 1;
    pl_desc->color_blend_.pAttachments = blend_state;

    static VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pl_desc->dynamic_state_.dynamicStateCount = 2;
    pl_desc->dynamic_state_.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo layout_cinfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    VkPipelineLayout pl_layout = {};
    VkPushConstantRange push_range = {};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.size = 2 * sizeof(VkDeviceAddress);
    layout_cinfo.pPushConstantRanges = &push_range;
    layout_cinfo.pushConstantRangeCount = 1;
    vkCreatePipelineLayout(ctx->device_, &layout_cinfo, nullptr, &pl_layout);

    construct_vk_shader(pl_desc->shaders_ + 0, ctx, "res/shaders/0.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    construct_vk_shader(pl_desc->shaders_ + 1, ctx, "res/shaders/0.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    pl_desc->cinfo_.stageCount = 2;

    return pl_layout;
}

DLL_EXPORT void cleaner(vk_ctx* ctx, vk_gfx_pl_desc* pl_desc)
{
    for (size_t i = 0; i < pl_desc->cinfo_.stageCount; i++)
    {
        destroy_vk_shader(pl_desc->shaders_ + i);
        memset(pl_desc->shaders_ + i, 0x0, sizeof(vk_shader));
    }
}