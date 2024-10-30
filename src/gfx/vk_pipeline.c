#include "vk_pipeline.h"

IMPL_OBJ_NEW(vk_shader, vk_ctx* ctx, const char* file_path, VkShaderStageFlags stage)
{
    cthis->ctx_ = ctx;
    FILE* f = fopen(file_path, "rb");
    if (!f)
    {
        fclose(f);
        return cthis;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    byte* spv = fi_alloc(byte, size);
    fread(spv, sizeof(*spv), size, f);
    fclose(f);

    VkShaderModuleCreateInfo module_info = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = size;
    module_info.pCode = (uint32_t*)spv;
    vkCreateShaderModule(ctx->device_, &module_info, nullptr, &cthis->module_);

    cthis->stage_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cthis->stage_info_.module = cthis->module_;
    cthis->stage_info_.stage = stage;
    cthis->stage_info_.pName = "main";

    fi_free(spv);
    return cthis;
}

IMPL_OBJ_DELETE(vk_shader)
{
    if (cthis->module_)
    {
        vkDestroyShaderModule(cthis->ctx_->device_, cthis->module_, nullptr);
    }
}

IMPL_OBJ_NEW(vk_gfx_pl_desc, vk_gfx_pl_configurator configurator, vk_gfx_pl_cleaner cleaner)
{
    cthis->configurator_ = configurator;
    cthis->cleaner_ = cleaner;
    cthis->atchms_.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    cthis->vtx_input_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    cthis->input_asm_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    cthis->tessellation_.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    cthis->viewport_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    cthis->rasterizer_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    cthis->multisample_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    cthis->depth_stencil_.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    cthis->color_blend_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cthis->dynamic_state_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    cthis->cinfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    cthis->cinfo_.pStages = cthis->stages_;
    cthis->cinfo_.pNext = &cthis->atchms_;
    cthis->cinfo_.pVertexInputState = &cthis->vtx_input_;
    cthis->cinfo_.pInputAssemblyState = &cthis->input_asm_;
    cthis->cinfo_.pTessellationState = &cthis->tessellation_;
    cthis->cinfo_.pViewportState = &cthis->viewport_;
    cthis->cinfo_.pRasterizationState = &cthis->rasterizer_;
    cthis->cinfo_.pMultisampleState = &cthis->multisample_;
    cthis->cinfo_.pDepthStencilState = &cthis->depth_stencil_;
    cthis->cinfo_.pColorBlendState = &cthis->color_blend_;
    cthis->cinfo_.pDynamicState = &cthis->dynamic_state_;
    return cthis;
}

IMPL_OBJ_DELETE_DEFAULT(vk_gfx_pl_desc)

VkPipeline vk_gfx_pl_desc_build(vk_gfx_pl_desc* cthis, vk_ctx* ctx, VkPipelineLayout* layout)
{
    *layout = cthis->configurator_(ctx, cthis);
    cthis->cinfo_.layout = *layout;

    for (size_t i = 0; i < cthis->cinfo_.stageCount; i++)
    {
        cthis->stages_[i] = cthis->shaders_[i].stage_info_;
    }

    VkPipeline pl = {};
    vkCreateGraphicsPipelines(ctx->device_, ctx->pipeline_cache_, 1, &cthis->cinfo_, nullptr, &pl);
    cthis->cleaner_(ctx, cthis);
    return pl;
}

IMPL_OBJ_NEW(vk_comp_pl_desc, vk_comp_pl_configurator configurator, vk_comp_pl_cleaner cleaner)
{
    cthis->configurator_ = configurator;
    cthis->cleaner_ = cleaner;
    cthis->cinfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    return cthis;
}

IMPL_OBJ_DELETE_DEFAULT(vk_comp_pl_desc)

VkPipeline vk_comp_pl_desc_build(vk_comp_pl_desc* cthis, vk_ctx* ctx, VkPipelineLayout* layout)
{
    *layout = cthis->configurator_(ctx, cthis);
    cthis->cinfo_.layout = *layout;

    VkPipeline pl = {};
    cthis->cinfo_.stage = cthis->shader_[0].stage_info_;
    vkCreateComputePipelines(ctx->device_, ctx->pipeline_cache_, 1, &cthis->cinfo_, nullptr, &pl);
    cthis->cleaner_(ctx, cthis);
    return pl;
}