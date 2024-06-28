#version 450 core

layout(location = 0) out vec4 position;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 albedo;
layout(location = 3) out vec4 specular;

layout(location = 0) in struct
{
    vec3 position;
    vec3 normal;
    vec3 uv;
} frag_data;

void main()
{
}