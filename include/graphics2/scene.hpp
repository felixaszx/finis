#ifndef ENGINE_SCENE_HPP
#define ENGINE_SCENE_HPP

#include "res_uploader.hpp"

namespace fi
{
    struct ResSceneNode
    {
        std::string name_ = "";
        glm::vec3 preset_translation_ = {0, 0, 0};
        glm::quat preset_rotation_ = {0, 0, 0, 0};
        glm::vec3 preset_scale_ = {1, 1, 1};
        glm::mat4 preset_transform_ = glm::identity<glm::mat4>(); // set by ResSceneDetails

        glm::vec3 translation_ = {0, 0, 0};
        glm::quat rotation_ = {0, 0, 0, 0};
        glm::vec3 scale_ = {1, 1, 1};
    };

    // support only 1 scene
    struct ResSceneDetails : private GraphicsObject
    {
      private:
        void update_scene_helper(const std::function<void(ResSceneNode& node, size_t node_idx)>& func, size_t curr,
                                 const glm::mat4& parent_transform);
        void update_scene_helper(size_t curr, const glm::mat4& parent_transform);

      public:
        struct SceneOffsets
        {
            vk::DeviceSize node_transform_idx_ = 0;
            vk::DeviceSize node_transform_ = 0;
        };

        std::string name_ = "";
        std::vector<ResSceneNode> nodes_{};                // indexed by node
        std::vector<std::vector<size_t>> node_children_{}; // indexed by node

        std::array<vk::DescriptorPoolSize, 1> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{}; // bind to vertex shader
        std::unique_ptr<Buffer<SceneOffsets, storage, seq_write>> buffer_{};
        std::vector<uint32_t> node_transform_idx_{}; // indexed by prim
        std::vector<glm::mat4> node_transform_{};
        // indexed by node and node_transform_idx_[prim], calculated by update_scene()

        std::vector<size_t> roots_;

        ResSceneDetails(const ResDetails& res_details);
        ~ResSceneDetails();

        // calculate node_transform after the callback
        void update_data();
        void update_scene(const std::function<void(ResSceneNode& node, size_t node_idx)>& func,
                          const glm::mat4& root_transform = glm::identity<glm::mat4>());
        void update_scene(const glm::mat4& root_transform = glm::identity<glm::mat4>());
        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set);
    };
}; // namespace fi

#endif // ENGINE_SCENE_HPP
