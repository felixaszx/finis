#version 460 core
#extension GL_EXT_nonuniform_qualifier : enable

// Declares

struct Material
{
    // pbr_data
    vec4 color_factor_; // alignment
    float metalic_;
    float roughtness_;
    uint color_texture_idx_;
    uint metalic_roughtness_; // alignment

    // emissive
    vec4 combined_emissive_factor; // [3] = emissive strength // alignment
    uint emissive_map_idx_;

    // occlusion  map
    uint occlusion_map_idx_;

    // alpha
    float alpha_value_;

    // normal
    float normal_scale_; // alignment
    uint normal_map_idx_;

    // anistropy
    float anistropy_rotation_;
    float anistropy_strength_;
    uint anistropy_map_idx_; // alginment

    // specular
    vec4 combined_spec_factor_; // [3] = specular strength // alginment
    uint spec_color_map_idx_;
    uint spec_map_idx_;

    // transmission
    float transmission_factor_;
    uint transmission_map_idx_; // alignment

    // volume
    vec4 combined_attenuation_; // [3] = attenuation distance // alignment
    float thickness_factor_;
    uint thickness_map_idx_;

    // sheen
    uint sheen_color_map_idx_;
    uint sheen_roughtness_map_idx_;         // alginment
    vec4 combined_sheen_color_factor_; // [3] = sheen roughtness factor
};

// Datas

layout(location = 0) out vec4 POSITION;

layout(location = 0) in struct
{
    vec3 position_;
    vec3 normal_;
    vec2 tex_coord_;
} FRAG_DATA;
layout(location = 3) in flat int MESH_IDX;

layout(set = 0, binding = 0) uniform sampler2D textures_arr[];
layout(std430, set = 0, binding = 1) readonly buffer MATERIALS_
{
    Material data_[];
}
MATERIALS;
layout(std430, set = 0, binding = 2) readonly buffer MATERIAL_IDXS_
{
    uint mat_idx_[];
}
MATERIAL_IDXS;

// Code

void main()
{
    Material mat = MATERIALS.data_[MATERIAL_IDXS.mat_idx_[MESH_IDX]];
    POSITION = texture(textures_arr[mat.color_texture_idx_], FRAG_DATA.tex_coord_);
}