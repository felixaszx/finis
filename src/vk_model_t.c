#include "vk_model_t.h"

IMPL_OBJ_NEW_DEFAULT(vk_material)
{
    memcpy(&this->color_factor_, (float[4]){1, 1, 1, 1}, sizeof(float[4]));
    memcpy(&this->emissive_factor_, (float[4]){0, 0, 0, 1}, sizeof(float[4]));
    memcpy(&this->sheen_color_factor_, (float[4]){0, 0, 0, 0}, sizeof(float[4]));
    memcpy(&this->specular_color_factor_, (float[4]){1, 1, 1, 1}, sizeof(float[4]));

    this->alpha_cutoff_ = 0;
    this->metallic_factor_ = 1.0f;
    this->roughness_factor_ = 1.0f;

    this->color_ = -1;
    this->metallic_roughness_ = -1;
    this->normal_ = -1;
    this->emissive_ = -1;
    this->occlusion_ = -1;

    this->anisotropy_rotation_ = 0;
    this->anisotropy_strength_ = 0;
    this->anisotropy_ = -1;

    this->specular_ = -1;
    this->spec_color_ = -1;

    this->sheen_color_ = -1;
    this->sheen_roughness_ = -1;
    return this;
}

IMPL_OBJ_NEW_DEFAULT(vk_morph)
{
    for (size_t i = 0; i < VK_MORPH_ATTRIB_COUNT; i++)
    {
        this->attrib_offsets_[i] = -1;
    }
    return this;
}

size_t vk_morph_get_attrib_size(vk_morph* this, vk_morph_attrib attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_MORPH_ATTRIB_COUNT] = {sizeof(float[3]), sizeof(float[3]), sizeof(float[4])};
    return ATTRIB_SIZES[attrib_type] * this->attrib_counts_[attrib_type];
}

IMPL_OBJ_NEW_DEFAULT(vk_prim)
{
    for (size_t i = 0; i < VK_PRIM_ATTRIB_COUNT; i++)
    {
        this->attrib_offsets_[i] = -1;
    }
    return this;
}

size_t vk_prim_get_attrib_size(vk_prim* this, vk_prim_attrib attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_PRIM_ATTRIB_COUNT] = {
        sizeof(uint32_t), sizeof(float[3]),    sizeof(float[3]), sizeof(float[4]),    sizeof(float[2]),
        sizeof(float[4]), sizeof(uint32_t[4]), sizeof(float[4]), sizeof(vk_material), sizeof(vk_morph)};
    return ATTRIB_SIZES[attrib_type] * this->attrib_counts_[attrib_type];
}