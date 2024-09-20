#version 460 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform sampler2D tex_arr[];

void main()
{
    vec2 tex_coord = {0, 0};
    vec4 aa = texture(tex_arr[0], tex_coord);
}