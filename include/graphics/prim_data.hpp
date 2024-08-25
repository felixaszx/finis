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

namespace fi
{
    inline static const uint32_t EMPTY = -1;
    inline static const size_t EMPTY_L = -1;

    struct PrimInfo
    {
        uint32_t positon_ = EMPTY;
        uint32_t normal_ = EMPTY;
        uint32_t tangent_ = EMPTY;
        uint32_t texcoord_ = EMPTY;
        uint32_t joints_ = EMPTY;
        uint32_t weights_ = EMPTY;

        uint32_t mesh_ = EMPTY;
        uint32_t material_ = EMPTY;
    };

    struct MorphInfo
    {
        uint32_t position_ = EMPTY;
        uint32_t normal_ = EMPTY;
        uint32_t tangent_ = EMPTY;

        uint32_t position_count_ = 0;
        uint32_t normal_count_ = 0;
        uint32_t tangent_count_ = 0;
    };

    struct MeshInfo
    {
        uint32_t node_ = EMPTY;
        uint32_t morph_weights_ = EMPTY;
        uint32_t joint_ = EMPTY;
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

}; // namespace fi

#endif // GRAPHICS_PRIM_DATA_HPP
