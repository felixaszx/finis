#ifndef GRAPHICS_RES_HOLDER_HPP
#define GRAPHICS_RES_HOLDER_HPP

#include "graphics.hpp"
#include "buffer.hpp"
#include "res_data.hpp"

namespace fi
{
    class ResSceneDetails : private GraphicsObject
    {
      private:
        std::vector<std::unique_ptr<gltf::Model>> models_{};

        std::vector<SceneNodeIdx> first_gltf_node_{};
        std::vector<uint32_t> gltf_node_count_{};

        std::vector<SceneNodeIdx> nodes_mapping_{}; // indexed by SceneNodeIdx
        std::vector<ResSceneNode> nodes_{};         // indexed by SceneNodeIdx in updating order
        std::vector<glm::mat4> node_transforms_{};  // indexed by SceneNodeIdx

        vk::DescriptorSet des_set_{}; // bind to vertex shader
        vk::DescriptorSetLayout set_layout_{};
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, storage, seq_write>> buffer_{};

        static std::array<vk::DescriptorPoolSize, 1> des_sizes_;

        static void build_scene_layer(SceneNodeIdx curr,                                      //
                                      std::vector<ResSceneNode>& loaded_nodes,                //
                                      const std::vector<std::vector<SceneNodeIdx>>& children, //
                                      std::vector<std::vector<SceneNodeIdx>>& layers);

      public:
        ~ResSceneDetails();

        void add_gltf(const std::filesystem::path& path);
        void update_data(size_t gltf_idx);
        ResSceneNode& index_node(SceneNodeIdx node_idx, size_t gltf_idx);
        uint32_t node_size(size_t gltf_idx);
        void update_gltf(size_t gltf_idx, const glm::mat4& root_transform = glm::identity<glm::mat4>());
        void allocate_gpu_res(vk::DescriptorPool des_pool);
        void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set);
        [[nodiscard]] const std::array<vk::DescriptorPoolSize, 1>& descriptor_pool_sizes() const;
        [[nodiscard]] const std::vector<std::unique_ptr<gltf::Model>>& models() const;
    };
}; // namespace fi

#endif // GRAPHICS_RES_HOLDER_HPP
