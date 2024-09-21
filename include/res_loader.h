#ifndef INCLUDE_RES_LOADER_H
#define INCLUDE_RES_LOADER_H

#include <cglm/cglm.h>
#include <stb/stb_image.h>

#include "tool.h"
#include "vk_mesh_t.h"
#include "third_party/gltf.h"

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
    byte* data_;
    uint32_t sampler_idx_;
} gltf_tex;

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

    size_t transform_count_;
    vk_prim_transform* transforms_;
} gltf_file;

DEFINE_OBJ(gltf_file, const char* file_path);

#endif // INCLUDE_RES_LOADER_H