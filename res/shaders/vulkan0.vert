#version 460 core
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require

#define EMPTY -1

struct PrimInfo
{
    uint first_position_;
    uint first_normal_;
    uint first_tangent_;
    uint first_texcoord_;
    uint first_color_;
    uint first_joint_;
    uint first_weight_;
    uint material_;

    uint mesh_idx_;
    uint morph_target_;
};

struct MorphTargetInfo // tbd
{
    uint first_position_;
    uint first_normal_;
    uint first_tangent_;

    uint position_morph_count_;
    uint normal_morph_count_;
    uint tangent_morph_count_;
};

struct MeshInfo
{
    uint node_;
    uint morph_weight_;

    uint first_joint;
    uint instances_;
};

// set 0
layout(std430, set = 0, binding = 0) readonly buffer _POSITION
{
    float POSITION[];
};
layout(std430, set = 0, binding = 1) readonly buffer _NORMAL
{
    float NORMAL[];
};
layout(std430, set = 0, binding = 2) readonly buffer _TANGENT
{
    float TANGENT[];
};
layout(std430, set = 0, binding = 3) readonly buffer _TEXCOORD
{
    float TEXCOORD[];
};
layout(std430, set = 0, binding = 4) readonly buffer _COLOR
{
    float COLOR[];
};
layout(std430, set = 0, binding = 5) readonly buffer _JOINT
{
    float JOINT[];
};
layout(std430, set = 0, binding = 6) readonly buffer _WEIGHT
{
    float WEIGHT[];
};
layout(std430, set = 0, binding = 7) readonly buffer _TARGET_POSITION
{
    float TARGET_POSITION[];
};
layout(std430, set = 0, binding = 8) readonly buffer _TARGET_NORMAL
{
    float TARGET_NORMAL[];
};
layout(std430, set = 0, binding = 9) readonly buffer _TARGET_TANGENT
{
    float TARGET_TANGENT[];
};
layout(std430, set = 0, binding = 11) readonly buffer _MESHES
{
    MeshInfo MESHES[];
};
layout(std430, set = 0, binding = 12) readonly buffer _MORPH_TARGETS
{
    MorphTargetInfo MORPH_TARGETS[];
};
layout(std430, set = 0, binding = 13) readonly buffer _PRIMITIVES
{
    PrimInfo PRIMITIVES[];
};

// set 1
layout(std430, set = 1, binding = 0) readonly buffer _NODE_TRANSFORMS
{
    mat4 NODE_TRANSFORMS[];
};

// out put
layout(location = 0) out struct1
{
    vec4 position_;
    vec3 normal_;
    vec3 tangent_;
    vec3 bitangent_;
    vec2 tex_coord_;
    vec4 color_;
}
FRAG_DATA;
layout(location = 6) out flat int PRIM_IDX;

layout(push_constant) uniform PUSHES_
{
    mat4 model_;
    mat4 view_;
    mat4 proj_;
}
PUSHES;

void main()
{
    PRIM_IDX = gl_DrawIDARB;
    PrimInfo prim_info = PRIMITIVES[PRIM_IDX];
    MeshInfo mesh_info = MESHES[prim_info.mesh_idx_];
    MorphTargetInfo morph_info = MORPH_TARGETS[prim_info.morph_target_];

    vec3 position = {POSITION[prim_info.first_position_ + gl_VertexIndex * 3 + 0], //
                     POSITION[prim_info.first_position_ + gl_VertexIndex * 3 + 1], //
                     POSITION[prim_info.first_position_ + gl_VertexIndex * 3 + 2]};
    vec3 normal = {0, 0, 0};
    vec4 tangent = {0, 0, 0, 1};
    vec2 texcoord = {0, 0};
    vec4 color = {1, 1, 1, 1};
    {
        if (prim_info.first_normal_ != EMPTY)
        {
            normal = vec3(NORMAL[prim_info.first_normal_ + gl_VertexIndex * 3 + 0],
                          NORMAL[prim_info.first_normal_ + gl_VertexIndex * 3 + 1],
                          NORMAL[prim_info.first_normal_ + gl_VertexIndex * 3 + 2]);
        }
        if (prim_info.first_tangent_ != EMPTY)
        {
            tangent = vec4(TANGENT[prim_info.first_tangent_ + gl_VertexIndex * 4 + 0],
                           TANGENT[prim_info.first_tangent_ + gl_VertexIndex * 4 + 1],
                           TANGENT[prim_info.first_tangent_ + gl_VertexIndex * 4 + 2],
                           TANGENT[prim_info.first_tangent_ + gl_VertexIndex * 4 + 3]);
        }
        if (prim_info.first_texcoord_ != EMPTY)
        {
            texcoord = vec2(TEXCOORD[prim_info.first_texcoord_ + gl_VertexIndex * 2 + 0],
                            TEXCOORD[prim_info.first_texcoord_ + gl_VertexIndex * 2 + 1]);
        }
        if (prim_info.first_color_ != EMPTY)
        {
            color = vec4(COLOR[prim_info.first_color_ + gl_VertexIndex * 4 + 0],
                         COLOR[prim_info.first_color_ + gl_VertexIndex * 4 + 1],
                         COLOR[prim_info.first_color_ + gl_VertexIndex * 4 + 2],
                         COLOR[prim_info.first_color_ + gl_VertexIndex * 4 + 3]);
        }
    }

    mat4 model = NODE_TRANSFORMS[mesh_info.node_];
    FRAG_DATA.position_ = model * vec4(position, 1.0);
    FRAG_DATA.normal_ = normalize(mat3(transpose(inverse(model))) * normal);
    FRAG_DATA.tangent_ = normalize(mat3(transpose(inverse(model))) * tangent.xyz);
    FRAG_DATA.bitangent_ = tangent.w * normalize(cross(FRAG_DATA.normal_, FRAG_DATA.tangent_));
    FRAG_DATA.tex_coord_ = texcoord;
    FRAG_DATA.color_ = color;

    gl_Position = PUSHES.proj_ * PUSHES.view_ * FRAG_DATA.position_;
}