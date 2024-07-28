#version 460 core
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec2 TEX_COORD;
// layout(location = 3) in uvec4 BONE_IDS;
// layout(location = 4) in vec4 BONE_WEIGHT;

layout(push_constant) uniform PUSHES_
{
    mat4 model_;
    mat4 view_;
    mat4 proj_;
}
PUSHES;

void main()
{
    gl_Position = PUSHES.proj_ * PUSHES.view_ * PUSHES.model_ * vec4(POSITION, 1.0);
}