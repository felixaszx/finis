#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "define.glslh"

struct vs_output
{
    vec3 position_;
    vec3 tangent_;
    vec3 bi_tangent_;
    vec2 tex_coord_;
};
layout(location = 0) in vs_output fs_in;

layout(location = 0) out vec4 COLOR;

void main()
{
    COLOR = vec4(1, 1, 1, 1);
}