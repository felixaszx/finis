#ifndef RESOURCES_GLTF_STRUCTURE_HPP
#define RESOURCES_GLTF_STRUCTURE_HPP

#include "gltf_file.hpp"
#include "graphics/prim_data.hpp"

namespace fi::res
{
    struct gltf_structure
    {
        std::vector<gfx::node_trs> nodes_{};            // not update sequentially
        std::vector<std::vector<uint32_t>> children_{}; // not update sequentially
        std::vector<uint32_t> seq_mapping_{};           // update sequentially
        std::vector<std::vector<float>> weights_{};
        std::vector<uint32_t> mesh_nodes_{}; // size of meshes

        gltf_structure(const gltf_file& file);
    };

    struct gltf_skins
    {
        std::vector<std::vector<uint32_t>> joints_;
        std::vector<uint32_t> mesh_joints_{}; // size of meshes
        gltf_skins(const gltf_file& file);
    };
}; // namespace fi::res

#endif // RESOURCES_GLTF_STRUCTURE_HPP