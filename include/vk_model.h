#ifndef INCLUDE_VK_MODEL_H
#define INCLUDE_VK_MODEL_H

#include "fi_vk.h"
#include "vk_model_t.h"

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
void vk_mesh_set_preset_transform(vk_mesh* this, mat4 transform);
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
bool vk_tex_arr_add_sampler(vk_tex_arr* this, VkSamplerCreateInfo* sampler_info);
bool vk_tex_arr_add_tex(vk_tex_arr* this,
                        VkCommandPool cmd_pool,
                        uint32_t sampler_idx,
                        byte* data,
                        size_t size,
                        const VkExtent3D* extent,
                        const VkImageSubresource* sub_res);

typedef struct vk_model_node
{
    vec3 translation_;
    versor rotation;
    vec3 scale_;
    mat4 preset_;

    mat4* output_;
} vk_model_node;

typedef struct vk_model_desc
{
    uint32_t node_layers_;
    uint32_t* layer_sizes_;
    vk_model_node** nodes_;

    mat4* output_;
} vk_model_desc;

DEFINE_OBJ(vk_model_desc, uint32_t node_limit);

#endif // INCLUDE_VK_MODEL_H