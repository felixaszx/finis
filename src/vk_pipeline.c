#include "vk_pipeline.h"

IMPL_OBJ_NEW(vk_shader, vk_ctx* ctx, const char* file_path, VkShaderStageFlags stage)
{
    this->ctx_ = ctx;
    FILE* f = fopen(file_path, "rb");
    if (!f)
    {
        fclose(f);
        return this;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    byte* spv = alloc(byte, size);
    fread(spv, sizeof(*spv), size, f);
    fclose(f);

    VkShaderModuleCreateInfo module_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = size;
    module_info.pCode = (uint32_t*)spv;
    vkCreateShaderModule(ctx->device_, &module_info, nullptr, &this->module_);

    this->stage_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    this->stage_info_.module = this->module_;
    this->stage_info_.stage = stage;
    this->stage_info_.pName = "main";

    ffree(spv);
    return this;
}

IMPL_OBJ_DELETE(vk_shader)
{
    if (this->module_)
    {
        vkDestroyShaderModule(this->ctx_->device_, this->module_, nullptr);
    }
}

IMPL_OBJ_NEW(vk_gfx_pl_desc, vk_gfx_pl_configurator configurator, vk_gfx_pl_cleaner cleaner)
{
    this->configurator_ = configurator;
    this->cleaner_ = cleaner;
    this->atchms_.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    this->vtx_input_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    this->input_asm_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    this->tessellation_.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    this->viewport_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    this->rasterizer_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    this->multisample_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    this->depth_stencil_.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    this->color_blend_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    this->dynamic_state_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    this->cinfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    this->cinfo_.pStages = this->stages_;
    this->cinfo_.pNext = &this->atchms_;
    this->cinfo_.pVertexInputState = &this->vtx_input_;
    this->cinfo_.pInputAssemblyState = &this->input_asm_;
    this->cinfo_.pTessellationState = &this->tessellation_;
    this->cinfo_.pViewportState = &this->viewport_;
    this->cinfo_.pRasterizationState = &this->rasterizer_;
    this->cinfo_.pMultisampleState = &this->multisample_;
    this->cinfo_.pDepthStencilState = &this->depth_stencil_;
    this->cinfo_.pColorBlendState = &this->color_blend_;
    this->cinfo_.pDynamicState = &this->dynamic_state_;
    return this;
}

IMPL_OBJ_DELETE_DEFAULT(vk_gfx_pl_desc)

VkPipeline vk_gfx_pl_desc_build(vk_gfx_pl_desc* this, vk_ctx* ctx, VkPipelineLayout* layout)
{
    *layout = this->configurator_(ctx, this);
    this->cinfo_.layout = *layout;

    for (size_t i = 0; i < this->cinfo_.stageCount; i++)
    {
        this->stages_[i] = this->shaders_[i].stage_info_;
    }

    VkPipeline pl = {};
    vkCreateGraphicsPipelines(ctx->device_, ctx->pipeline_cache_, 1, &this->cinfo_, nullptr, &pl);
    this->cleaner_(ctx, this);
    return pl;
}

IMPL_OBJ_NEW(vk_comp_pl_desc, vk_comp_pl_configurator configurator, vk_comp_pl_cleaner cleaner)
{
    this->configurator_ = configurator;
    this->cleaner_ = cleaner;
    this->cinfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    return this;
}

IMPL_OBJ_DELETE_DEFAULT(vk_comp_pl_desc)

VkPipeline vk_comp_pl_desc_build(vk_comp_pl_desc* this, vk_ctx* ctx, VkPipelineLayout* layout)
{
    *layout = this->configurator_(ctx, this);
    this->cinfo_.layout = *layout;

    VkPipeline pl = {};
    this->cinfo_.stage = this->shader_[0].stage_info_;
    vkCreateComputePipelines(ctx->device_, ctx->pipeline_cache_, 1, &this->cinfo_, nullptr, &pl);
    this->cleaner_(ctx, this);
    return pl;
}