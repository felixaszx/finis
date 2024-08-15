#ifndef GRAPHICS_RES_DATA_HPP
#define GRAPHICS_RES_DATA_HPP

#include "graphics.hpp"

namespace fi
{
#define EMPTY -1
    using IdxIdx = uint32_t;
    using VtxIdx = uint32_t;
    using MorphTargetIdx = uint32_t;
    using MaterialIdx = uint32_t;
    using MeshIdx = uint32_t;
    using NodeIdx = uint32_t;
    using InstanceIdx = uint32_t;
    using MorphWeightIdx = uint32_t;
    using TexIdx = uint32_t;
    using SamplerIdx = uint32_t;
    using PrimIdx = uint32_t;

    class TSIdx
    {
      private:
        uint32_t idx_ = EMPTY;

      public:
        TSIdx(const uint32_t& idx = EMPTY)
            : idx_(idx)
        {
        }
        inline operator uint32_t&() { return idx_; }
        inline void operator++() { idx_++; }
        inline void operator--() { idx_--; }
        inline void operator+=(const uint32_t& offset) { idx_ += offset; }
        inline void operator-=(const uint32_t& offset) { idx_ -= offset; }
        inline uint32_t operator+(const uint32_t& offset) const { return idx_ + offset; }
        inline uint32_t operator-(const uint32_t& offset) const { return idx_ - offset; }
        inline void operator+=(const size_t& offset) { idx_ += offset; }
        inline void operator-=(const size_t& offset) { idx_ -= offset; }
        inline uint32_t operator+(const size_t& offset) const { return idx_ + offset; }
        inline uint32_t operator-(const size_t& offset) const { return idx_ - offset; }
        inline bool operator==(const size_t& cmp) const { return idx_ == cmp; }
        inline bool operator==(const uint32_t& cmp) const { return idx_ == cmp; }
    };

    class TSIdxIdx : public TSIdx
    {
    };
    class TSVtxIdx : public TSIdx
    {
    };
    class TSMorphTargetIdx : public TSIdx
    {
    };
    class TSMaterialIdx : public TSIdx
    {
    };
    class TSMeshIdx : public TSIdx
    {
    };
    class TSNodeIdx : public TSIdx
    {
    };
    class TSInstanceIdx : public TSIdx
    {
    };
    class TSMorphWeightIdx : public TSIdx
    {
    };
    class TSTexIdx : public TSIdx
    {
    };
    class TSSamplerIdx : public TSIdx
    {
    };

    struct PrimInfo
    {
        VtxIdx first_position_ = EMPTY;
        VtxIdx first_normal_ = EMPTY;
        VtxIdx first_tangent_ = EMPTY;
        VtxIdx first_texcoord_ = EMPTY;
        VtxIdx first_color_ = EMPTY;
        VtxIdx first_joint_ = EMPTY;
        VtxIdx first_weight_ = EMPTY;
        MaterialIdx material_ = EMPTY;

        MeshIdx mesh_idx_ = EMPTY;
        MorphTargetIdx morph_target_ = EMPTY;
    };

    struct MorphTargetInfo // tbd
    {
        VtxIdx first_position_ = EMPTY;
        VtxIdx first_normal_ = EMPTY;
        VtxIdx first_tangent_ = EMPTY;

        uint32_t position_morph_count_ = 0;
        uint32_t normal_morph_count_ = 0;
        uint32_t tangent_morph_count_ = 0;
    };

    struct MeshInfo
    {
        NodeIdx node_ = EMPTY;
        MorphWeightIdx morph_weight_ = EMPTY;

        NodeIdx first_joint = EMPTY;
        InstanceIdx instances_ = EMPTY;
    };

    struct alignas(16) MaterialInfo
    {
        glm::vec4 color_factor_ = {1, 1, 1, 1};
        glm::vec4 emissive_factor_ = {0, 0, 0, 1};       // [3] = emissive strength
        glm::vec4 sheen_color_factor_ = {0, 0, 0, 0};    // [3] = sheen roughtness factor
        glm::vec4 specular_color_factor_ = {1, 1, 1, 1}; // [3] = specular factor

        float alpha_cutoff_ = 0; // -1 means blend, 0 means opaque, otherwise means mask
        float metalic_factor_ = 1.0f;
        float roughtness_factor_ = 1.0f;
        TexIdx color_ = EMPTY;
        TexIdx metalic_roughtness_ = EMPTY;

        TexIdx normal_ = EMPTY;
        TexIdx emissive_ = EMPTY;
        TexIdx occlusion_ = EMPTY;

        float anisotropy_rotation_ = 0;
        float anisotropy_strength_ = 0;
        TexIdx anisotropy_ = EMPTY;

        TexIdx specular_ = EMPTY;
        TexIdx spec_color_ = EMPTY;

        TexIdx sheen_color_ = EMPTY;
        TexIdx sheen_roughtness_ = EMPTY;
    };

    using NodeTransform = glm::mat4;

    struct InstancesInfo
    {
        // tbd
    };
}; // namespace fi

#endif // GRAPHICS_RES_DATA_HPP
