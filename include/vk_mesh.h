#ifndef INCLUDE_VK_MESH_H
#define INCLUDE_VK_MESH_H

#include "vk.h"

#define VK_PRIM_ATTRIB_COUNT 9

typedef uint32_t uvec4[4];
typedef enum vk_prim_attrib_type
{
    INDEX,
    POSITION,
    NORMAL,
    TANGENT,
    TEXCOORD,
    COLOR,
    JOINTS,
    WEIGHTS,
    MATERIAL
} vk_prim_attrib_type;

typedef struct vk_material
{
    vec4 color_factor_;
    vec4 emissive_factor_;       // [3] = emissive strength
    vec4 sheen_color_factor_;    // [3] = sheen roughness factor
    vec4 specular_color_factor_; // [3] = specular factor

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

typedef struct vk_prim
{
    char name_[512];
    size_t attrib_counts_[VK_PRIM_ATTRIB_COUNT];
    byte_offset attrib_datas_[VK_PRIM_ATTRIB_COUNT]; // offset inside vk_mesh::buffer_
} vk_prim;

void init_vk_prim(vk_prim* prim);
size_t vk_prim_get_attrib_size(vk_prim* prim, vk_prim_attrib_type attrib_type);

typedef struct vk_mesh
{
    char name_[512];
    uint32_t prim_count_;
    vk_prim* prims_;

    vk_ctx* ctx_;
    VkBuffer buffer_;
    VmaAllocation alloc_;
    VkDeviceSize mem_limit_;
} vk_mesh;

DEFINE_OBJ(vk_mesh, vk_ctx* ctx, VkDeviceSize mem_limit);

#endif // INCLUDE_VK_MESH_H
