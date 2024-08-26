#version 460 core
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Declares

#define EMPTY -1

struct MaterialInfo
{
    vec4 color_factor_;
    vec4 emissive_factor_;       // [3] = emissive strength
    vec4 sheen_color_factor_;    // [3] = sheen roughtness factor
    vec4 specular_color_factor_; // [3] = specular factor

    float alpha_cutoff_; // -1 means blend, 0 means opaque, otherwise means mask
    float metalic_factor_;
    float roughtness_factor_;
    uint color_;
    uint metalic_roughtness_;

    uint normal_;
    uint emissive_;
    uint occlusion_;

    float anisotropy_rotation_;
    float anisotropy_strength_;
    uint anisotropy_;

    uint specular_;
    uint spec_color_;

    uint sheen_color_;
    uint sheen_roughtness_;
};
// Datas

layout(location = 0) out vec4 POSITION;
layout(location = 1) out vec4 NORMAL;
layout(location = 2) out vec4 COLOR;
layout(location = 3) out vec4 SPECULAR;

layout(location = 0) in struct
{
    vec4 position_;
    vec3 normal_;
    vec3 tangent_;
    vec3 bitangent_;
    vec2 tex_coord_;
    vec4 color_;
} FRAG_DATA;
layout(location = 6) in flat uint MATERIAL_IDX;

// Code

void main()
{
}