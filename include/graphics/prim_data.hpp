/**
 * @file res_data.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#ifndef GRAPHICS_PRIM_DATA_HPP
#define GRAPHICS_PRIM_DATA_HPP

#include "graphics.hpp"

namespace fi::graphics
{
    // all pointer use as pointer to uint32_t
    inline static const uint32_t EMPTY = -1;
    inline static const uint64_t EMPTY_L = -1;

    struct PrimInfo
    {
        enum Attribute
        {
            POSITON,
            NORMAL,
            TANGENT,
            TEXCOORD,
            JOINTS,
            WEIGHTS,
            MESH,
            MATERIAL
        };

        uint64_t positon_ = EMPTY_L;  // vec3[]
        uint64_t normal_ = EMPTY_L;   // vec3[]
        uint64_t tangent_ = EMPTY_L;  // vec4[]
        uint64_t texcoord_ = EMPTY_L; // vec2[]
        uint64_t joints_ = EMPTY_L;   // uvec4[]
        uint64_t weights_ = EMPTY_L;  // vec4[]

        uint64_t mesh_ = EMPTY_L;     // MeshInfo*
        uint64_t material_ = EMPTY_L; // MaterialInfo*

        uint64_t& get_attrib_ptr(Attribute attrib)
        {
            switch (attrib)
            {
                case POSITON:
                    return positon_;
                case NORMAL:
                    return normal_;
                case TANGENT:
                    return tangent_;
                case TEXCOORD:
                    return texcoord_;
                case JOINTS:
                    return joints_;
                case WEIGHTS:
                    return weights_;
                case MESH:
                    return mesh_;
                case MATERIAL:
                    return material_;
            }
        }
    };

    struct MorphInfo
    {
        uint64_t position_ = EMPTY_L; // vec3[]
        uint64_t normal_ = EMPTY_L;   // vec3[]
        uint64_t tangent_ = EMPTY_L;  // vec4[]

        uint64_t position_count_ = 0; // scalar
        uint64_t normal_count_ = 0;   // scalar
        uint64_t tangent_count_ = 0;  // scalar
    };

    struct MeshInfo
    {
        uint64_t node_ = EMPTY_L;          // mat4*
        uint64_t morph_weights_ = EMPTY_L; // float[]
        uint64_t joint_ = EMPTY_L;         // uint32_t[]
    };

    struct MaterialInfo
    {
        glm::vec4 color_factor_ = {1, 1, 1, 1};
        glm::vec4 emissive_factor_ = {0, 0, 0, 1};       // [3] = emissive strength
        glm::vec4 sheen_color_factor_ = {0, 0, 0, 0};    // [3] = sheen roughtness factor
        glm::vec4 specular_color_factor_ = {1, 1, 1, 1}; // [3] = specular factor

        float alpha_cutoff_ = 0; // -1 means blend, 0 means opaque, otherwise means mask
        float metalic_factor_ = 1.0f;
        float roughtness_factor_ = 1.0f;
        uint32_t color_ = EMPTY;
        uint32_t metalic_roughtness_ = EMPTY;

        uint32_t normal_ = EMPTY;
        uint32_t emissive_ = EMPTY;
        uint32_t occlusion_ = EMPTY;

        float anisotropy_rotation_ = 0;
        float anisotropy_strength_ = 0;
        uint32_t anisotropy_ = EMPTY;

        uint32_t specular_ = EMPTY;
        uint32_t spec_color_ = EMPTY;

        uint32_t sheen_color_ = EMPTY;
        uint32_t sheen_roughtness_ = EMPTY;
    };

#define size_as_uint32_t(bytes)     (bytes / sizeof(uint32_t))
#define sizeof_as_uint32_t(type)    (size_as_uint32_t(sizeof(type)))
#define sizeof_arr_as_uint32_t(arr) (arr.size() * sizeof_as_uint32_t(arr[0]))

}; // namespace fi

#endif // GRAPHICS_PRIM_DATA_HPP
