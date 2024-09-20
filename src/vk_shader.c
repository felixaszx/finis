#include "vk_shader.h"

IMPL_OBJ_NEW(vk_shader, vk_ctx* ctx, const char* file_path, VkShaderStageFlags stage)
{
    this->ctx_ = ctx;
    FILE* f = fopen(file_path, "rb");
    if (!f)
    {
        return this;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    byte* spv = alloc(char, size);
    fread(spv, sizeof(byte), size, f);

    VkShaderModuleCreateInfo module_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = size;
    module_info.pCode = (uint32_t*)spv;
    vkCreateShaderModule(ctx->device_, &module_info, nullptr, &this->module_);

    this->stage_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    this->stage_info_.module = this->module_;
    this->stage_info_.stage = stage;
    this->stage_info_.pName = "main";
    return this;
}

IMPL_OBJ_DELETE(vk_shader)
{
    ffree(this->layout_.binding_size_);
    vkDestroyShaderModule(this->ctx_->device_, this->module_, nullptr);
}