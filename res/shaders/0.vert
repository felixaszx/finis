#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

#define VK_MORPH_ATTRIB_COUNT 3
#define VK_PRIM_ATTRIB_COUNT 11
#define INDEX                0
#define POSITION             1
#define NORMAL               2
#define TANGENT              3
#define TEXCOORD             4
#define COLOR                5
#define JOINTS               6
#define WEIGHTS              7
#define MATERIAL             8
#define MORPH                9
#define TRANSFORM            10
#define DRAW_CALL_SIZE       16
#define PRIM_INFO_SIZE       176
#define PRIM_INFO_STRIDE     (DRAW_CALL_SIZE + PRIM_INFO_SIZE)

struct vk_mesh_joint
{
    uint32_t joint_;
    mat4 inv_binding_;
};

struct vk_prim_transform
{
    uint32_t node_idx_;
    uint32_t first_joint_;
    uint32_t morph_weights_;
};

struct vk_prim
{
    uint64_t attrib_counts_[VK_PRIM_ATTRIB_COUNT];
    uint64_t attrib_address_[VK_PRIM_ATTRIB_COUNT];
};

struct vk_morph
{
    uint64_t attrib_counts_[VK_MORPH_ATTRIB_COUNT];
    uint64_t attrib_offsets_[VK_MORPH_ATTRIB_COUNT];
};

struct VkDrawIndirectCommand
{
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct prim_combo
{
    VkDrawIndirectCommand draw_call_;
    vk_prim prim;
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer ptr_t
{
    int8_t val_;
};
layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer vec3_arr_t
{
    vec3 val_[];
};
layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer uint32_t_arr_t
{
    uint32_t val_[];
};
layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer prim_combo_arr_t
{
    prim_combo val_[];
};
layout(std430, push_constant) uniform _PUSHED
{
    prim_combo_arr_t PRIM_COMBO_ARR;
};

void main()
{
    prim_combo prim = PRIM_COMBO_ARR.val_[gl_DrawID];
    uint32_t idx = uint32_t_arr_t(prim.prim.attrib_address_[INDEX]).val_[gl_VertexIndex];
    vec3 position = vec3_arr_t(prim.prim.attrib_address_[POSITION]).val_[idx];
    gl_Position.xyz = position;
    gl_Position.w = 1;
}