#version 460 core
#extension GL_EXT_nonuniform_qualifier : enable

// Declares

#define NO_TEXTURE -1
struct Material
{
    vec4 color_factor_;
    vec4 emissive_factor_;    // [3] = emissive strength
    vec4 sheen_color_factor_; // [3] = sheen roughtness factor
    vec4 spec_factor_;        // [3] = place holder

    float alpha_cutoff_;
    float metalic_;
    float roughtness_;
    uint color_texture_idx_;
    uint metalic_roughtness_;

    uint normal_map_idx_;
    uint emissive_map_idx_;
    uint occlusion_map_idx_;

    float anistropy_rotation_;
    float anistropy_strength_;
    uint anistropy_map_idx_;

    uint spec_map_idx_;
    uint spec_color_map_idx_;

    uint sheen_color_map_idx_;
    uint sheen_roughtness_map_idx_;
};

// Datas

layout(location = 0) out vec4 POSITION;
layout(location = 1) out vec4 NORMAL;
layout(location = 2) out vec4 COLOR;

layout(location = 0) in struct
{
    vec4 position_;
    vec3 normal_;
    vec3 tangent_;
    vec3 bitangent_;
    vec2 tex_coord_;
} FRAG_DATA;
layout(location = 5) in flat int MESH_IDX;

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
    COLOR = mat.color_factor_;
    POSITION = FRAG_DATA.position_;
    NORMAL = vec4(FRAG_DATA.normal_, 1);

    // texture mapping
    if (mat.color_texture_idx_ != NO_TEXTURE)
    {
        COLOR *= texture(textures_arr[mat.color_texture_idx_], FRAG_DATA.tex_coord_);
        if (COLOR.a < mat.alpha_cutoff_)
        {
            discard;
        }
    }

    if (mat.normal_map_idx_ != NO_TEXTURE)
    {
        vec3 mapped_normal = texture(textures_arr[mat.normal_map_idx_], FRAG_DATA.tex_coord_).rgb;
        mapped_normal = normalize(mapped_normal * 2.0 - 1.0);
        NORMAL.rgb = normalize(mat3(FRAG_DATA.tangent_, FRAG_DATA.bitangent_, FRAG_DATA.normal_) * mapped_normal);
    }
}