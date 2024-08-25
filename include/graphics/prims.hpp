#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

#include "graphics.hpp"
#include "prim_data.hpp"

namespace fi
{
    struct Primitives
    {
      public:
        struct AttribSetter
        {
            Primitives* prims_ = nullptr;

            AttribSetter& add_positions(const std::vector<glm::vec3>& positions);
            AttribSetter& add_normals(const std::vector<glm::vec3>& normals);
            AttribSetter& add_tangents(const std::vector<glm::vec4>& tangents);
            AttribSetter& add_texcoords(const std::vector<glm::vec2>& texcoords);
            AttribSetter& add_joints(const std::vector<glm::uvec4>& joints);
            AttribSetter& add_weights(const std::vector<glm::vec4>& weights);
            AttribSetter& add_morph_positions(const std::vector<glm::vec3>& positions);
            AttribSetter& add_morph_normals(const std::vector<glm::vec3>& normals);
            AttribSetter& add_morph_tangents(const std::vector<glm::vec4>& tangents);

            AttribSetter& set_mesh_idx(const std::vector<uint32_t>& mesh_idxs);
            AttribSetter& set_material_idx(const std::vector<uint32_t>& material_idxs);
            AttribSetter& add_mesh_infos(const std::vector<MeshInfo>& mesh_infos);
            AttribSetter& add_material_infos(const std::vector<MaterialInfo>& material_infos);
        } setter_;

      private:
        // min alignment in data_buffer is 4 byte
        vk::DeviceSize curr_size_ = 0;
        const vk::DeviceSize max_size_ = 0;
        vk::Buffer data_buffer_{}; // buffer 0
        vk::DeviceAddress data_address_{};
        vma::Allocation data_alloc_{};

        uint32_t prim_count_ = 0;
        const uint32_t max_prims_ = 0;
        vk::Buffer prim_buffer_{}; // buffer 1
        vk::DeviceAddress prim_address_{};
        vma::Allocation prim_alloc_{};

      public:
        Primitives(uint32_t max_data_size, uint32_t max_prim_count);
        ~Primitives();

        AttribSetter& add_primitives(const std::vector<vk::DrawIndexedIndirectCommand>& draw_calls);
    };
}; // namespace fi

#endif // GRAPHICS_PRIMS_HPP