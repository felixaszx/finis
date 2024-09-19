#ifndef INCLUDE_VK_MESH_H
#define INCLUDE_VK_MESH_H

#include "fi_vk.h"
#include "vk_mesh_t.h"

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
DEFINE_OBJ_DELETE(vk_mesh);
vk_prim* vk_mesh_add_prim(vk_mesh* this);
void vk_mesh_add_prim_attrib(vk_mesh* this, vk_prim* prim, vk_prim_attrib attrib, void* data, size_t count);
void vk_mesh_add_prim_morph_attrib(vk_mesh* this, vk_morph* morph, vk_morph_attrib attrib, void* data, size_t count);
void vk_mesh_free_staging(vk_mesh* this);
void vk_mesh_flush_staging(vk_mesh* this);
void vk_mesh_alloc_device_mem(vk_mesh* this, VkCommandPool pool);
void vk_mesh_draw_prims(vk_mesh* this, VkCommandBuffer cmd);

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
DEFINE_OBJ_DELETE(vk_tex_arr);
bool vk_tex_arr_add_sampler(vk_tex_arr* this, VkSamplerCreateInfo* sampler_info);
bool vk_tex_arr_add_tex(vk_tex_arr* this,
                        VkCommandPool cmd_pool,
                        uint32_t sampler_idx,
                        byte* data,
                        size_t size,
                        const VkExtent3D* extent,
                        const VkImageSubresource* sub_res);

typedef struct vk_mesh_desc
{
    // linearized
    uint32_t node_size_;
    vk_mesh_node* nodes_;
    mat4* output_;

    uint32_t layer_sizes_;
    uint32_t* layers_;

    vk_ctx* ctx_;
    byte* mapping_;
    VkBuffer buffer_;
    VmaAllocation alloc_;
    VkDeviceAddress address_;
} vk_mesh_desc;

DEFINE_OBJ(vk_mesh_desc, vk_ctx* ctx, uint32_t node_size_);
DEFINE_OBJ_DELETE(vk_mesh_desc);
void vk_mesh_desc_update(vk_mesh_desc* this, mat4 root_trans);
void vk_mesh_desc_set_layer(vk_mesh_desc* this, uint32_t layer_size);
void vk_mesh_desc_alloc_device_mem(vk_mesh_desc* this, VkCommandPool pool);

typedef struct vk_mesh_joint
{
    uint32_t joint_;
    mat4 inv_binding_;
} vk_mesh_joint;

typedef struct vk_mesh_skin
{
    uint32_t joint_size_;
    vk_mesh_joint* joints_;

    vk_ctx* ctx_;
    VkBuffer buffer_;
    VmaAllocation alloc_;
    VkDeviceAddress address_;
} vk_mesh_skin;

DEFINE_OBJ(vk_mesh_skin, vk_ctx* ctx, uint32_t joint_size_);
DEFINE_OBJ_DELETE(vk_mesh_skin);
void vk_mesh_skin_alloc_device_mem(vk_mesh_skin* this, VkCommandPool pool);

#endif // INCLUDE_VK_MESH_H
