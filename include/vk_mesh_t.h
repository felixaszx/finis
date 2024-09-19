#ifndef INCLUDE_VK_MESH_T_H
#define INCLUDE_VK_MESH_T_H

#include "fi_vk.h"

typedef struct vk_material
{
    float color_factor_[4];
    float emissive_factor_[4];       // [3] = emissive strength
    float sheen_color_factor_[4];    // [3] = sheen roughness factor
    float specular_color_factor_[4]; // [3] = specular factor

    float alpha_cutoff_; // -1 means blend, 0 means opaque, otherwise means mask
    float metallic_factor_;
    float roughness_factor_;

    uint32_t color_;
    uint32_t metallic_roughness_;
    uint32_t normal_;
    uint32_t emissive_;
    uint32_t occlusion_;

    float anisotropy_rotation_;
    float anisotropy_strength_;
    uint32_t anisotropy_;

    uint32_t specular_;
    uint32_t spec_color_;

    uint32_t sheen_color_;
    uint32_t sheen_roughness_;
} vk_material;

DEFINE_OBJ_DEFAULT(vk_material);

#define VK_MORPH_ATTRIB_COUNT 3
typedef enum vk_morph_attrib
{
    MORPH_POSITION,
    MORPH_NORMAL,
    MORPH_TANGENT
} vk_morph_attrib;

typedef struct vk_morph
{
    VkDeviceSize attrib_counts_[VK_MORPH_ATTRIB_COUNT];
    VkDeviceSize attrib_offsets_[VK_MORPH_ATTRIB_COUNT]; // offset inside vk_mesh::buffer_
} vk_morph;

DEFINE_OBJ_DEFAULT(vk_morph);
size_t vk_morpj_get_attrib_size(vk_morph* this, vk_morph_attrib attrib_type);

#define VK_PRIM_ATTRIB_COUNT 11
typedef enum vk_prim_attrib
{
    INDEX,
    POSITION,
    NORMAL,
    TANGENT,
    TEXCOORD,
    COLOR,
    JOINTS,
    WEIGHTS,
    MATERIAL,
    MORPH,

    TRANSFORM,
} vk_prim_attrib;

typedef struct vk_prim
{
    VkDeviceSize attrib_counts_[VK_PRIM_ATTRIB_COUNT];
    VkDeviceSize attrib_offsets_[VK_PRIM_ATTRIB_COUNT]; // offset inside vk_mesh::buffer_
} vk_prim;

DEFINE_OBJ_DEFAULT(vk_prim);
size_t vk_prim_get_attrib_size(vk_prim* this, vk_prim_attrib attrib_type);

typedef struct vk_prim_transform
{
    uint32_t node_idx_;
    uint32_t first_joint_;
    uint32_t morph_weights_;
} vk_prim_transform;
DEFINE_OBJ_DEFAULT(vk_prim_transform);

#endif // INCLUDE_VK_MESH_T_H
