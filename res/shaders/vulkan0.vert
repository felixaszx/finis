#version 460 core
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#define EMPTY -1

struct PrimInfo
{
    uint64_t positon_;  // vec3[]
    uint64_t normal_;   // vec3[]
    uint64_t tangent_;  // vec4[]
    uint64_t texcoord_; // vec2[]
    uint64_t joints_;   // uvec4[]
    uint64_t weights_;  // vec4[]

    uint64_t mesh_;     // MeshInfo*
    uint64_t material_; // MaterialInfo*
};

struct MorphInfo
{
    uint64_t position_; // vec3[]
    uint64_t normal_;   // vec3[]
    uint64_t tangent_;  // vec4[]

    uint64_t position_count_; // scalar
    uint64_t normal_count_;   // scalar
    uint64_t tangent_count_;  // scalar
};

struct MeshInfo
{
    uint64_t node_;          // mat4*
    uint64_t morph_weights_; // float[]
    uint64_t joint_;         // uint32_t[]
};

layout(buffer_reference, scalar) readonly buffer RawBuffer { uint arr_[]; };
layout(buffer_reference, scalar) readonly buffer PrimInfoBuffer { PrimInfo arr_[]; };
layout(buffer_reference, scalar) readonly buffer MorphInfoBuffer { MorphInfo arr_[]; };
layout(buffer_reference, scalar) readonly buffer MeshInfoBuffer { MeshInfo arr_[]; };
layout(buffer_reference, scalar) readonly buffer Vec2Buffer { vec3 arr_[]; };
layout(buffer_reference, scalar) readonly buffer Vec3Buffer { vec3 arr_[]; };
layout(buffer_reference, scalar) readonly buffer Vec4Buffer { vec4 arr_[]; };
layout(buffer_reference, scalar) readonly buffer UVec4Buffer { vec4 arr_[]; };
layout(buffer_reference, scalar) readonly buffer Mat4Buffer { mat4 arr_[]; };

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
layout(location = 6) out flat int MATERIAL_IDX;

void main() { MATERIAL_IDX = gl_DrawIDARB; }