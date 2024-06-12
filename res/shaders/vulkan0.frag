#version 450 core
#extension GL_EXT_multiview : enable
#extension GL_ARB_shader_draw_parameters : enable

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

layout(binding = 1) uniform sampler2D albe_tex;
layout(binding = 2) uniform sampler2D spec_tex;
layout(binding = 3) uniform sampler2D opac_tex;
layout(binding = 4) uniform sampler2D ambi_tex;
layout(binding = 5) uniform sampler2D norm_tex;
layout(binding = 6) uniform sampler2D emis_tex;

void main()
{

}