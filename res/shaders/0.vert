#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "define.glslh"
#include "prim.glslh"

//
// logic block
//

layout(scalar, buffer_reference, buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer vec3_arr_t
{
    vec3 val_[];
};

layout(scalar, buffer_reference, buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer vec2_arr_t
{
    vec2 val_[];
};

layout(scalar,
       buffer_reference,
       buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer uint32_t_arr_t
{
    uint32_t val_[];
};

layout(scalar,
       buffer_reference,
       buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer prim_combo_arr_t
{
    prim_combo val_[];
};

layout(std430, push_constant) uniform _PUSHED
{
    prim_combo_arr_t PRIM_COMBO_ARR;
    ptr_t DATA;
}
PUSHED;

void main()
{
    prim_combo prim = PUSHED.PRIM_COMBO_ARR.val_[gl_DrawID];
    uint32_t idx = uint32_t_arr_t(prim.prim.attrib_address_[INDEX]).val_[gl_VertexIndex];
    vec3 position = vec3_arr_t(prim.prim.attrib_address_[POSITION]).val_[idx];

    gl_Position.xyz = position;
    gl_Position.w = 1;
}