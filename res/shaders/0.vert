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
    readonly buffer mat4_arr_t
{
    mat4 val_[]; // transformation of each node
};

layout(scalar, buffer_reference, buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer vk_prim_transform_ptr_t
{
    vk_prim_transform val_; // transformation of each node
};

layout(scalar, buffer_reference, buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer vk_mesh_joint_arr_t
{
    vk_mesh_joint val_[];
};

layout(scalar, buffer_reference, buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer vec4_arr_t
{
    vec4 val_[];
};

layout(scalar, buffer_reference, buffer_reference_align = VK_MESH_STORAGE_BUFFER_ALIGNMENT) //
    readonly buffer uvec4_arr_t
{
    uvec4 val_[];
};

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
    mat4_arr_t NODES;
    vk_mesh_joint_arr_t SKIN;
    mat4 VIEW_MAT;
    mat4 PROJECTION_MAT;
}
PUSHED;

void main()
{
    prim_combo prim_combo = PUSHED.PRIM_COMBO_ARR.val_[gl_DrawID];
    vk_prim_transform prim_transform = vk_prim_transform_ptr_t(prim_combo.prim.attrib_address_[TRANSFORM]).val_;
    uint32_t_arr_t idxs = uint32_t_arr_t(prim_combo.prim.attrib_address_[INDEX]);
    uint32_t vtx_id = idxs.val_[gl_VertexIndex];

    mat4 node_transform = PUSHED.NODES.val_[prim_transform.node_idx_];

    vec3 position = vec3_arr_t(prim_combo.prim.attrib_address_[POSITION]).val_[vtx_id];
    vec3 normal = vec3(0);
    vec4 tangent = vec4(0);
    vec2 texcoord = vec2(0);
    vec4 color = vec4(1, 1, 1, 1);
    uvec4 joints = uvec4(-1, -1, -1, -1);
    vec4 weights = vec4(1, 1, 1, 1);

    {
        if (prim_combo.prim.attrib_counts_[NORMAL] != 0)
        {
            normal = vec3_arr_t(prim_combo.prim.attrib_address_[NORMAL]).val_[vtx_id];
        }
        if (prim_combo.prim.attrib_counts_[TANGENT] != 0)
        {
            tangent = vec4_arr_t(prim_combo.prim.attrib_address_[TANGENT]).val_[vtx_id];
        }
        if (prim_combo.prim.attrib_counts_[TEXCOORD] != 0)
        {
            texcoord = vec2_arr_t(prim_combo.prim.attrib_address_[TEXCOORD]).val_[vtx_id];
        }
        if (prim_combo.prim.attrib_counts_[COLOR] != 0)
        {
            color = vec4_arr_t(prim_combo.prim.attrib_address_[COLOR]).val_[vtx_id];
        }
        if (prim_combo.prim.attrib_counts_[JOINTS] != 0)
        {
            joints = uvec4_arr_t(prim_combo.prim.attrib_address_[JOINTS]).val_[vtx_id];
        }
        if (prim_combo.prim.attrib_counts_[WEIGHTS] != 0)
        {
            weights = vec4_arr_t(prim_combo.prim.attrib_address_[WEIGHTS]).val_[vtx_id];
        }
    }

    gl_Position = vec4(joints.x, joints.y, joints.z, joints.w);
}