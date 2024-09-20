#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

struct vk_mesh_joint
{
    uint32_t joint_;
    mat4 inv_binding_;
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer prim_data
{
    uint8_t arr[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer node_data
{
    mat4 arr[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer skin_data
{
    vk_mesh_joint arr[];
};

layout(std430, push_constant) uniform _PUSHED
{
    prim_data PRIM_DATA;
    node_data NODE_DATA;
    skin_data SKIN_DATA;
};

void main()
{
    uint8_t a = PRIM_DATA.arr[1];
    float b = PRIM_DATA.arr[1];
    gl_Position.x = gl_DrawID;
}