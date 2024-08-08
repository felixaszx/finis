#version 460 core
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec4 TANGENT;
layout(location = 3) in vec2 TEX_COORD;
layout(location = 4) in vec4 COLOR;
layout(location = 5) in uvec4 JOINT_IDX;
layout(location = 6) in vec4 JOINT_WEIGHT;

layout(location = 0) out struct
{
    vec4 position_;
    vec3 normal_;
    vec3 tangent_;
    vec3 bitangent_;
    vec2 tex_coord_;
    vec4 color_;
} FRAG_DATA;
layout(location = 6) out flat int PRIM_IDX;

layout(push_constant) uniform PUSHES_
{
    mat4 model_;
    mat4 view_;
    mat4 proj_;
}
PUSHES;

layout(std430, set = 1, binding = 0) readonly buffer JOINT_IDXS_
{
    uint joint_idx_[];
}
JOINT_IDXS;
layout(std430, set = 1, binding = 1) readonly buffer JOINT_INV_MAT_
{
    mat4 mat_[];
}
JOINT_INV_MAT;
layout(std430, set = 1, binding = 2) readonly buffer JOINT_
{
    uint node_idx_[];
}
JOINT;

layout(std430, set = 2, binding = 0) readonly buffer NODE_IDXS_
{
    uint node_idx_[];
}
NODE_IDXS;
layout(std430, set = 2, binding = 1) readonly buffer NODE_TRANSFORM_
{
    mat4 mat_[];
}
NODE_TRANSFORM;

void main()
{
    PRIM_IDX = gl_DrawIDARB;
    mat4 skin_transform = NODE_TRANSFORM.mat_[NODE_IDXS.node_idx_[PRIM_IDX]];
    uint skin_idx = JOINT_IDXS.joint_idx_[PRIM_IDX];

    if (skin_idx != -1)
    {
        skin_transform = mat4(0.0);
        for (int i = 0; i < 4; i++)
        {
            float joint_weight = JOINT_WEIGHT[i];
            uint joint_idx = skin_idx + JOINT_IDX[i];
            mat4 joint_mat = NODE_TRANSFORM.mat_[JOINT.node_idx_[joint_idx]];
            mat4 joint_inv_mat = JOINT_INV_MAT.mat_[joint_idx];
            skin_transform += joint_weight * joint_mat * joint_inv_mat;
        }
    }

    mat4 model = skin_transform;
    FRAG_DATA.position_ = model * vec4(POSITION, 1.0);
    FRAG_DATA.normal_ = normalize(mat3(transpose(inverse(model))) * NORMAL);
    FRAG_DATA.tangent_ = normalize(mat3(transpose(inverse(model))) * TANGENT.xyz);
    FRAG_DATA.bitangent_ = TANGENT.w * normalize(cross(FRAG_DATA.normal_, FRAG_DATA.tangent_));
    FRAG_DATA.tex_coord_ = TEX_COORD;
    FRAG_DATA.color_ = COLOR;

    gl_Position = PUSHES.proj_ * PUSHES.view_ * FRAG_DATA.position_;
}