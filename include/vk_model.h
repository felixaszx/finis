#ifndef INCLUDE_VK_MODEL_H
#define INCLUDE_VK_MODEL_H

#include "vk.h"

#define VK_PRIM_ATTRIB_COUNT 9

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
    MATERIAL
} vk_prim_attrib;

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

typedef struct vk_prim
{
    size_t attrib_counts_[VK_PRIM_ATTRIB_COUNT];
    byte_offset attrib_offsets_[VK_PRIM_ATTRIB_COUNT]; // offset inside vk_mesh::buffer_
} vk_prim;

DEFINE_OBJ_DEFAULT(vk_prim);
size_t vk_prim_get_attrib_size(vk_prim* this, vk_prim_attrib attrib_type);

typedef struct vk_mesh
{
    char name_[512];
    uint32_t prim_limit_;
    uint32_t prim_size_;
    vk_prim* prims_;

    vk_ctx* ctx_;
    VkBuffer buffer_;
    VmaAllocation alloc_;
    VkDeviceAddress address_;

    VkDeviceSize dc_offset_;
    VkDrawIndirectCommand* draw_calls_;

    byte* mapping_;
    VkBuffer staging_;
    VkDeviceSize mem_limit_;
    VkDeviceSize mem_size_;
    VmaAllocation staging_alloc_;
} vk_mesh;

DEFINE_OBJ(vk_mesh, vk_ctx* ctx, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit);
vk_prim* vk_mesh_add_prim(vk_mesh* this);
void vk_mesh_add_prim_attrib(vk_mesh* this, vk_prim* prim, vk_prim_attrib attrib, void* data, size_t count);
void vk_mesh_free_staging(vk_mesh* this);
void vk_mesh_flush_staging(vk_mesh* this);
void vk_mesh_alloc_device_mem(vk_mesh* this, VkCommandPool pool);
void vk_mesh_draw_prims(vk_mesh* this, VkCommandBuffer cmd);

typedef struct vk_model
{
    char name_[512];
    uint32_t mesh_limit_;
    uint32_t size_;
    vk_mesh* meshes_;

    vk_ctx* ctx_;
} vk_model;

DEFINE_OBJ(vk_model, vk_ctx* ctx, const char* name, uint32_t mesh_limit);
vk_mesh* vk_model_add_mesh(vk_model* this, const char* name, VkDeviceSize mem_limit, uint32_t prim_limit);

typedef struct vk_tex_arr
{
    uint32_t tex_size_;
    uint32_t tex_limit_;
    VkImage* texs_;
    VkImageView* views_;
    VmaAllocation* allocs_;
    VkDescriptorImageInfo* desc_infos_;

    uint32_t sampler_size_;
    uint32_t sampler_limit_;
    VkSampler* samplers_;

    vk_ctx* ctx_;
} vk_tex_arr;

DEFINE_OBJ(vk_tex_arr, vk_ctx* ctx, uint32_t tex_limit, uint32_t sampler_limit);

bool vk_tex_arr_add_sampler(vk_tex_arr* this, VkSampler sampler);
bool vk_tex_arr_add_tex(vk_tex_arr* this,
                        VkCommandPool cmd_pool,
                        uint32_t sampler_idx,
                        byte* data,
                        size_t size,
                        const VkExtent3D* extent,
                        const VkImageSubresource* sub_res);

#endif // INCLUDE_VK_MODEL_H