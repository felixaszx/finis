#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "define.glslh"
#include "prim.glslh"

layout(location = 0) out vec4 POSITION_OUT;
layout(location = 1) out vec4 NORMAL_OUT;
layout(location = 2) out vec4 COLOR_OUT;
layout(location = 3) out vec4 SPEC_OUT;

layout(scalar,
       buffer_reference,
       buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer prim_combo_arr_t
{
    prim_combo val_[];
};
layout(scalar,
       buffer_reference,
       buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer vk_material_ptr_t
{
    vk_material val_;
};

layout(location = 0) in flat uint prim_idx_out;
layout(location = 1) in flat ptr_t prim_combos_out;
layout(location = 2) in vert_stage vert_out;

layout(set = 0, binding = 0) uniform sampler2D tex_arr[];

void main()
{
    prim_combo prim_combo = prim_combo_arr_t(prim_combos_out).val_[prim_idx_out];
    vk_material material = vk_material_ptr_t(prim_combo.prim.attrib_address_[MATERIAL]).val_;
    COLOR_OUT = material.color_factor_ * texture(tex_arr[material.color_], vert_out.texcoord_);
}