#version 460 core
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec2 TEX_COORD;
layout(location = 3) in uvec4 BONE_IDS;
layout(location = 4) in vec4 BONE_WEIGHT;

layout(location = 0) out struct
{
    vec3 position_;
    vec3 normal_;
    vec2 tex_coord_;
} FRAG_DATA;
layout(location = 3) out flat int MAT_IDX;

layout(push_constant) uniform PUSHES_
{
    mat4 model_;
    mat4 view_;
    mat4 proj_;
}
PUSHES;

void main()
{
    MAT_IDX = gl_DrawIDARB;
}