#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "define.glslh"

layout(location = 0) out vec4 POSITION;
layout(location = 1) out vec4 NORMAL;
layout(location = 2) out vec4 COLOR;
layout(location = 3) out vec4 SPEC;

layout(scalar,
       buffer_reference,
       buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer prim_combo_arr_t
{
    prim_combo val_[];
};

layout(location = 0) in flat uint prim_idx_out;
layout(location = 1) in flat ptr_t prim_combos_out;
layout(location = 2) in vert_stage vert_out;

void main()
{
    COLOR = vec4(vert_out.position_, 1);
}