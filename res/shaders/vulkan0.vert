#version 450 core
#extension GL_EXT_multiview : enable
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 uv;
layout(location = 3) in mat4 instance_model;

layout(push_constant) uniform constants_
{
    mat4 view_;
    mat4 proj_;
}
constants;

layout(location = 0) out struct
{
    vec3 position;
    vec3 normal;
    vec3 uv;
} frag_data;

void main()
{
}