#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 COLOR;

void main()
{
    COLOR = vec4(1, 1, 1, 1);
}