#version 460 core
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec4 TANGENT;
layout(location = 3) in vec2 TEX_COORD;
layout(location = 4) in vec4 COLOR;
layout(location = 5) in uvec4 BONE_IDS;
layout(location = 6) in vec4 BONE_WEIGHT;

layout(location = 0) out struct
{
    vec4 position_;
    vec3 normal_;
    vec3 tangent_;
    vec3 bitangent_;
    vec2 tex_coord_;
} FRAG_DATA;
layout(location = 5) out flat int MESH_IDX;

layout(push_constant) uniform PUSHES_
{
    mat4 model_;
    mat4 view_;
    mat4 proj_;
}
PUSHES;

void main()
{
    MESH_IDX = gl_DrawIDARB;
    FRAG_DATA.position_ = PUSHES.model_ * vec4(POSITION, 1.0);
    FRAG_DATA.normal_ = normalize(mat3(transpose(inverse(PUSHES.model_))) * NORMAL);
    FRAG_DATA.tangent_ = normalize(mat3(transpose(inverse(PUSHES.model_))) * TANGENT.xyz);
    FRAG_DATA.bitangent_ = normalize(cross(FRAG_DATA.normal_, FRAG_DATA.tangent_));
    FRAG_DATA.tex_coord_ = TEX_COORD;
    gl_Position = PUSHES.proj_ * PUSHES.view_ * FRAG_DATA.position_;
}