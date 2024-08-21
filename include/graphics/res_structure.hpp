/**
 * @file res_structure.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#ifndef GRAPHICS_RES_STRUCTURE_HPP
#define GRAPHICS_RES_STRUCTURE_HPP

#include "res_loader.hpp"

namespace fi
{
    class ResStructure : private GraphicsObject
    {
      public:
        struct NodeInfo
        {
            uint32_t depth_ = 0;
            NodeIdx parent_idx_ = EMPTY;
            NodeIdx self_idx_ = EMPTY;
            std::string name_ = "";

            glm::vec3 translation_ = {0, 0, 0};
            glm::quat rotation_ = {0, 0, 0, 1};
            glm::vec3 scale_ = {1, 1, 1};
        };

      private:
        // helpers
        size_t old_node_count_ = 0;
        std::vector<TSNodeIdx> first_node_{};

        // storages
        std::vector<TSNodeIdx> node_mappings_{};
        std::vector<NodeInfo> nodes_{};

      public:
        struct BufferOffsets
        {
            vk::DeviceSize transforms_ = 0;
            vk::DeviceSize target_weights_ = 0;
        };

        std::array<vk::DescriptorPoolSize, 1> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{};
        std::unique_ptr<Buffer<BufferOffsets, storage, seq_write, host_coherent>> buffer_;

        std::vector<NodeTransform> transforms_{}; // binding 0
        std::vector<float> morph_weight_{};       // binding 1

        ResStructure(ResDetails& res_details);
        ~ResStructure();

        NodeInfo& index_node(TSNodeIdx idx, size_t gltf_idx);
        void update_data(size_t gltf_idx = EMPTY);
        void update_structure(const glm::mat4& transform = glm::identity<glm::mat4>(), size_t gltf_idx = EMPTY);
        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind_res(vk::CommandBuffer cmd, vk::PipelineBindPoint bind_point, //
                      vk::PipelineLayout pipeline_layout, uint32_t des_set);
        [[nodiscard]] const std::vector<NodeInfo>& nodes() const { return nodes_; }
    };
}; // namespace fi

#endif // GRAPHICS_RES_STRUCTURE_HPP
