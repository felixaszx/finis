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
layout(location = 1) out vec4 NORMAL;
layout(location = 2) out vec4 COLOR;
layout(location = 3) out vec4 METALIC_ROUGHTNESS;

layout(location = 0) in struct
{
    vec3 position_;
    vec3 normal_;
    vec2 tex_coord_;
} FRAG_DATA;
layout(location = 3) in flat int MAT_IDX;

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(std430, set = 0, binding = 1) readonly buffer MATERIALS_
{
    Material data_[];
}
MATERIALS;

// Code

void main()
{
}