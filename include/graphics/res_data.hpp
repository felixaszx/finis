#ifndef GRAPHICS_RES_DATA_HPP
#define GRAPHICS_RES_DATA_HPP

#include "graphics.hpp"

namespace fi
{
    using PrimIdx = uint32_t;
    using SceneNodeIdx = uint32_t;
    using MaterialIdx = uint32_t;
    using SkinIdx = uint32_t;
    using MeshIdx = uint32_t;
    using JointIdx = uint32_t;
    using TexIdx = uint32_t;

    struct PrimVtx
    {
        glm::vec3 position_ = {0, 0, 0};
        glm::vec3 normal_ = {0, 0, 0};
        glm::vec4 tangent_ = {0, 0, 0, 1};
        glm::vec2 tex_coord_ = {0, 0};
        glm::vec4 color_ = {1, 1, 1, 1};
        glm::uvec4 joint_ = {-1, -1, -1, -1};
        glm::vec4 weight_ = {0, 0, 0, 0};
    };

    struct PrimDetails
    {
        MeshIdx mesh_idx_ = 0;
        MaterialIdx material_idx_ = 0;
    };

    struct alignas(16) PrimMaterial
    {
        glm::vec4 color_factor_ = {1, 1, 1, 1};
        glm::vec4 emissive_factor_ = {0, 0, 0, 1};    // [3] = emissive strength
        glm::vec4 sheen_factor_ = {0, 0, 0, 0}; // [3] = sheen roughtness factor
        glm::vec4 specular_factor_ = {1, 1, 1, 1};        // [3] = place holder

        float alpha_cutoff_ = 0;
        float metalic_factor_ = 1.0f;
        float roughtness_factor_ = 1.0f;
        TexIdx color_ = 0;
        TexIdx metalic_roughness_ = ~0;

        TexIdx normal_ = ~0;
        TexIdx emissive_ = ~0;
        TexIdx occlusion_ = ~0;

        float anistropy_rotation_ = 0;
        float anistropy_strength_ = 0;
        TexIdx anistropy_ = ~0;

        TexIdx specular_ = ~0;
        TexIdx specular_color_ = ~0;

        TexIdx sheen_color_ = ~0;
        TexIdx sheen_roughtness_ = ~0;
    };

    struct MeshDetails
    {
        SceneNodeIdx node_idx_ = 0;
        SkinIdx skin_idx_ = -1;
        uint32_t instance_details_idx_ = -1; // tbd
    };

    struct ResMesh
    {
        std::string name_ = "";
        MeshDetails* details_ = nullptr;
        PrimIdx first_prim_ = 0;
        uint32_t prim_count_ = 0;
    };

    struct ResSkeleton
    {
        std::string name_ = "";
        SceneNodeIdx joint_idx_ = 0;
        uint32_t joint_count_ = 0;
    };

    struct ResSceneNode
    {
        uint32_t depth_ = 0;
        SceneNodeIdx parent_idx = -1;
        SceneNodeIdx node_idx = 0;

        std::string name_ = "";
        glm::vec3 translation_ = {0, 0, 0};
        glm::quat rotation_ = {0, 0, 0, 1};
        glm::vec3 scale_ = {1, 1, 1};
        glm::mat4 preset_ = glm::identity<glm::mat4>(); // set by ResSceneDetails
    };
}; // namespace fi

#endif // GRAPHICS_RES_DATA_HPP
