#version 460 core
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

// Declares

#define EMPTY -1

struct PrimInfo
{
    uint first_position_;
    uint first_normal_;
    uint first_tangent_;
    uint first_texcoord_;
    uint first_color_;
    uint first_joint_;
    uint first_weight_;
    uint material_;

    uint mesh_idx_;
    uint morph_target_;
};

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
layout(location = 6) in flat int PRIM_IDX;

layout(std430, set = 0, binding = 13) readonly buffer _PRIMITIVES { PrimInfo PRIMITIVES[]; };
layout(std430, set = 0, binding = 14) readonly buffer _MATERIALS { MaterialInfo MATERIALS[]; };
layout(set = 0, binding = 15) uniform sampler2D textures_arr[];

// Code

void main()
{
    PrimInfo prim_info = PRIMITIVES[PRIM_IDX];
    MaterialInfo mat = MATERIALS[prim_info.material_];
    POSITION = FRAG_DATA.position_;
    COLOR = FRAG_DATA.color_ * mat.color_factor_;
    NORMAL = vec4(FRAG_DATA.normal_, 1);

    // texture mapping
    if (mat.color_ != EMPTY)
    {
        COLOR *= texture(textures_arr[mat.color_], FRAG_DATA.tex_coord_);
    }

    if (mat.normal_ != EMPTY)
    {
        vec3 mapped_normal = texture(textures_arr[mat.normal_], FRAG_DATA.tex_coord_).rgb;
        mapped_normal = normalize(mapped_normal * 2.0 - 1.0);
        NORMAL.rgb = normalize(mat3(FRAG_DATA.tangent_, FRAG_DATA.bitangent_, FRAG_DATA.normal_) * mapped_normal);
    }

    // blending
}