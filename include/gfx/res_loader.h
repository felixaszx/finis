#ifndef INCLUDE_RES_LOADER_H
#define INCLUDE_RES_LOADER_H

#include <cglm/cglm.h>
#include <stb/stb_image.h>

#include "fi_tool.h"
#include "gltf.h"
#include "vk_mesh_t.h"

typedef uint32_t uvec4[4];

typedef struct gltf_prim
{
    char name_[512];
    uint32_t idx_count_;
    uint32_t* idx_;

    size_t vtx_count_;
    vec3* position;
    vec3* normal_;
    vec4* tangent_;
    vec2* texcoord_;
    vec4* color_;
    uvec4* joint_;
    vec4* weight_;
    vk_material* material_;
    vk_morph* morph_;
    vk_prim_transform* transform_;
} gltf_prim;

typedef struct gltf_tex
{
    char name_[512];

    int width_;
    int height_;
    int channel_;
    uint32_t levels_;
    byte* data_;
    uint32_t sampler_idx_;
} gltf_tex;
size_t gltf_tex_size(gltf_tex* this);
VkExtent3D gltf_tex_extent(gltf_tex* this);

// glb only
typedef struct gltf_file
{
    cgltf_data* data_;

    size_t prim_count_;
    gltf_prim* prims_;

    size_t material_count_;
    vk_material* materials_;

    uint32_t sampler_count_;
    VkSamplerCreateInfo* sampler_cinfos_;

    size_t tex_count_;
    gltf_tex* texs_;
} gltf_file;

DEFINE_OBJ(gltf_file, const char* file_path);

typedef struct gltf_desc
{
    uint32_t node_count_;
    uint32_t* mapping_;   // original order
    vk_mesh_node* nodes_; // linearized, indexed by mapping_[idx]

    uint32_t mesh_count_;
    uint32_t* prim_offset_;
    vk_prim_transform* transform_;
    vk_prim_transform** prim_transform_; // a mapping of transform, size is gltf_file::prim_count_
} gltf_desc;

DEFINE_OBJ(gltf_desc, gltf_file* file);

#define GLTF_FRAME_CHANNEL_COUNT 4
typedef enum gltf_frame_channel
{
    GLTF_FRAME_CHANNEL_T,
    GLTF_FRAME_CHANNEL_R,
    GLTF_FRAME_CHANNEL_S,
    GLTF_FRAME_CHANNEL_W,
} gltf_frame_channel;

// time step in ms
typedef size_t gltf_ms;
typedef struct gltf_frame
{
    gltf_ms step_count_[GLTF_FRAME_CHANNEL_COUNT];
    gltf_ms* time_stamps_[GLTF_FRAME_CHANNEL_COUNT];
    T* data_[GLTF_FRAME_CHANNEL_COUNT]; // casted before interporlation

    // extra
    size_t w_per_morph_;
} gltf_frame;

DEFINE_OBJ(gltf_frame, size_t t_count, size_t r_count, size_t s_count, size_t w_count, size_t w_per_morph);
void gltf_frame_sample(gltf_frame* this, gltf_frame_channel channel, gltf_ms time_pt, T* dst); // lerp only

typedef struct gltf_anim
{

} gltf_anim;

#endif // INCLUDE_RES_LOADER_H