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
    uint JOINT[];
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
// binding 10 is draw call
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
layout(std430, set = 1, binding = 1) readonly buffer _MORPH_WEIGHT
{
    float MORPH_WEIGHT[];
};

// set 2
layout(std430, set = 2, binding = 0) readonly buffer _SKIN_JOINTS
{
    uint SKIN_JOINTS[];
};
layout(std430, set = 2, binding = 1) readonly buffer _INV_BINDINGS
{
    mat4 INV_BINDINGS[];
};

// out put
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

#define GET_VTX(s)             (s * gl_VertexIndex)
#define GET_VEC2(arr, offset)  vec2(arr[offset + 0], arr[offset + 1])
#define GET_VEC3(arr, offset)  vec3(arr[offset + 0], arr[offset + 1], arr[offset + 2])
#define GET_VEC4(arr, offset)  vec4(arr[offset + 0], arr[offset + 1], arr[offset + 2], arr[offset + 3])
#define GET_UVEC2(arr, offset) uvec2(arr[offset + 0], arr[offset + 1])
#define GET_UVEC3(arr, offset) uvec3(arr[offset + 0], arr[offset + 1], arr[offset + 2])
#define GET_UVEC4(arr, offset) uvec4(arr[offset + 0], arr[offset + 1], arr[offset + 2], arr[offset + 3])

void main()
{
    PRIM_IDX = gl_DrawIDARB;
    PrimInfo prim_info = PRIMITIVES[PRIM_IDX];
    MeshInfo mesh_info = MESHES[prim_info.mesh_idx_];
    MorphTargetInfo morph_info = MORPH_TARGETS[prim_info.morph_target_];

    // pull vtx datas
    vec3 position = GET_VEC3(POSITION, prim_info.first_position_ + GET_VTX(3));
    vec3 normal = {0, 0, 0};
    vec4 tangent = {0, 0, 0, 1};
    vec2 texcoord = {0, 0};
    vec4 color = {1, 1, 1, 1};
    {
        if (prim_info.first_normal_ != EMPTY)
        {
            normal = GET_VEC3(NORMAL, prim_info.first_normal_ + GET_VTX(3));
        }
        if (prim_info.first_tangent_ != EMPTY)
        {
            tangent = GET_VEC4(TANGENT, prim_info.first_tangent_ + GET_VTX(4));
        }
        if (prim_info.first_texcoord_ != EMPTY)
        {
            texcoord = GET_VEC2(TEXCOORD, prim_info.first_texcoord_ + GET_VTX(2));
        }
        if (prim_info.first_color_ != EMPTY)
        {
            color = GET_VEC4(COLOR, prim_info.first_color_ + GET_VTX(4));
        }
    }

    // calculate morph targets
    {
        if (morph_info.first_position_ != EMPTY)
        {
            uint first_morph_idx = morph_info.first_position_ //
                                   + GET_VTX(3) * morph_info.position_morph_count_;
            for (int t = 0; t < morph_info.position_morph_count_; t++)
            {
                position += MORPH_WEIGHT[mesh_info.morph_weight_ + t] //
                            * GET_VEC3(TARGET_POSITION, first_morph_idx + 3 * t);
            }
        }

        if (morph_info.first_normal_ != EMPTY)
        {
            uint first_morph_idx = morph_info.first_normal_ //
                                   + GET_VTX(3) * morph_info.normal_morph_count_;
            for (int t = 0; t < morph_info.position_morph_count_; t++)
            {
                normal += MORPH_WEIGHT[mesh_info.morph_weight_ + t] //
                          * GET_VEC3(TARGET_NORMAL, first_morph_idx + 3 * t);
            }
        }

        if (morph_info.first_tangent_ != EMPTY)
        {
            uint first_morph_idx = morph_info.first_tangent_ //
                                   + GET_VTX(4) * morph_info.tangent_morph_count_;
            for (int t = 0; t < morph_info.position_morph_count_; t++)
            {
                tangent += MORPH_WEIGHT[mesh_info.morph_weight_ + t] //
                           * GET_VEC4(TARGET_TANGENT, first_morph_idx + 4 * t);
            }
        }
    }

    mat4 model = NODE_TRANSFORMS[mesh_info.node_];
    // calculate skeleton animation
    if (mesh_info.first_joint != EMPTY)
    {
        model = mat4(0);
        uvec4 joint_ids = GET_UVEC4(JOINT, prim_info.first_joint_ + GET_VTX(4));
        for (int i = 0; i < 4; i++)
        {
            model += GET_VEC4(WEIGHT, prim_info.first_weight_ + GET_VTX(4))[i]            //
                     * NODE_TRANSFORMS[SKIN_JOINTS[mesh_info.first_joint + joint_ids[i]]] //
                     * INV_BINDINGS[mesh_info.first_joint + joint_ids[i]];
        }
    }

    mat3 normal_correction = mat3(transpose(inverse(model)));
    FRAG_DATA.position_ = model * vec4(position, 1.0);
    FRAG_DATA.normal_ = normalize(normal_correction * normal);
    FRAG_DATA.tangent_ = normalize(normal_correction * tangent.xyz);
    FRAG_DATA.bitangent_ = tangent.w * normalize(cross(FRAG_DATA.normal_, FRAG_DATA.tangent_));
    FRAG_DATA.tex_coord_ = texcoord;
    FRAG_DATA.color_ = color;

    gl_Position = PUSHES.proj_ * PUSHES.view_ * FRAG_DATA.position_;
}