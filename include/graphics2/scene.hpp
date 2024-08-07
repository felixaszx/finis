#ifndef ENGINE_SCENE_HPP
#define ENGINE_SCENE_HPP

#include "res_uploader.hpp"

namespace fi
{
    struct ResSceneNode
    {
        std::string name_ = "";
        glm::vec3 translation_ = {0, 0, 0};
        glm::quat rotation_ = {0, 0, 0, 1};
        glm::vec3 scale_ = {1, 1, 1};
    };

    // support only 1 scene
    struct ResSceneDetails
    {
      private:
        void update_scene_helper(const std::function<void(ResSceneNode& node, size_t node_idx)>& func, size_t curr,
                                 const glm::mat4& parent_transform);

      public:
        std::string name_ = "";
        std::vector<ResSceneNode> nodes_{};                // indexed by node
        std::vector<std::vector<size_t>> node_children_{}; // indexed by node

        std::vector<uint32_t> node_transform_idx_{}; // indexed by prim
        std::vector<glm::mat4> node_transform_{};    // indexed by node and node_transform_idx_[prim], calculated

        std::vector<size_t> roots_;

        ResSceneDetails(const ResDetails& res_details);

        // calculate node_transform after the callback
        void update_scene(const std::function<void(ResSceneNode& node, size_t node_idx)>& func);
    };
}; // namespace fi

#endif // ENGINE_SCENE_HPP
