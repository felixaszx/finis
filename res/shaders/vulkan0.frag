#version 460 core
#extension GL_EXT_nonuniform_qualifier : enable

// Declares

struct Material
{
    // pbr datas
    vec4 color_factor_;
    float metalic_;
    float roughtness_;
    uint color_texture_idx_;
    uint metalic_roughtness_texture_idx_;
};

// Datas

layout(location = 0) out vec4 POSITION;

layout(location = 0) in struct
{
    vec3 position_;
    vec3 normal_;
    vec2 tex_coord_;
} FRAG_DATA;
layout(location = 3) in flat int MESH_IDX;

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(std430, set = 0, binding = 1) readonly buffer MATERIALS_
{
    Material data_[];
}
MATERIALS;
layout(std430, set = 0, binding = 2) readonly buffer MATERIAL_IDXS_
{
    uint mat_idx_[];
}
MATERIAL_IDXS;

// Code

void main()
{
    POSITION = texture(textures[MATERIALS.data_[MATERIAL_IDXS.mat_idx_[MESH_IDX]].color_texture_idx_], //
                       FRAG_DATA.tex_coord_);
}