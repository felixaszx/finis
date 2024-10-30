#include "vk_mesh_t.h"

IMPL_OBJ_NEW_DEFAULT(vk_material)
{
    memcpy(&cthis->color_factor_, (float[4]){1, 1, 1, 1}, sizeof(float[4]));
    memcpy(&cthis->emissive_factor_, (float[4]){0, 0, 0, 1}, sizeof(float[4]));
    memcpy(&cthis->sheen_color_factor_, (float[4]){0, 0, 0, 0}, sizeof(float[4]));
    memcpy(&cthis->specular_color_factor_, (float[4]){1, 1, 1, 1}, sizeof(float[4]));

    cthis->alpha_cutoff_ = 0;
    cthis->metallic_factor_ = 1.0f;
    cthis->roughness_factor_ = 1.0f;

    cthis->color_ = -1;
    cthis->metallic_roughness_ = -1;
    cthis->normal_ = -1;
    cthis->emissive_ = -1;
    cthis->occlusion_ = -1;

    cthis->anisotropy_rotation_ = 0;
    cthis->anisotropy_strength_ = 0;
    cthis->anisotropy_ = -1;

    cthis->specular_ = -1;
    cthis->spec_color_ = -1;

    cthis->sheen_color_ = -1;
    cthis->sheen_roughness_ = -1;
    return cthis;
}

size_t vk_morph_get_attrib_size(vk_morph* morph, vk_morph_attrib attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_MORPH_ATTRIB_COUNT] = {sizeof(float[3]), sizeof(float[3]), sizeof(float[3])};
    return ATTRIB_SIZES[attrib_type] * morph->attrib_counts_[attrib_type];
}

size_t vk_prim_get_attrib_size(vk_prim* cthis, vk_prim_attrib attrib_type)
{
    const static size_t ATTRIB_SIZES[VK_PRIM_ATTRIB_COUNT] = {
        sizeof(uint32_t),    sizeof(float[3]), sizeof(float[3]),    sizeof(float[4]),
        sizeof(float[2]),    sizeof(float[4]), sizeof(uint32_t[4]), sizeof(float[4]),
        sizeof(vk_material), sizeof(vk_morph), sizeof(VkDeviceSize)};
    return ATTRIB_SIZES[attrib_type] * cthis->attrib_counts_[attrib_type];
}

IMPL_OBJ_NEW_DEFAULT(vk_prim_transform)
{
    cthis->node_idx_ = -1;
    cthis->first_joint_ = -1;
    cthis->target_count_ = -1;
    return cthis;
}

IMPL_OBJ_NEW_DEFAULT(vk_mesh_node)
{
    glm_vec3_copy(GLM_VEC3_ONE, cthis->scale_);
    glm_quat_identity(cthis->rotation);
    glm_mat4_identity(cthis->preset_);
    cthis->parent_idx_ = -1;
    return cthis;
}
IMPL_OBJ_DELETE_DEFAULT(vk_mesh_node)