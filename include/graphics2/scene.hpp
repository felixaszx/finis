#ifndef ENGINE_SCENE_HPP
#define ENGINE_SCENE_HPP

#include "res_uploader.hpp"

namespace fi
{
    struct ResSceneNode
    {
        glm::vec3 position_;
        glm::quat rotation_;
        glm::vec3 scale_;
    };

    struct ResSceneDetails
    {
        std::vector<ResSceneNode> all_nodes_{};            // indexed by node
        std::vector<std::vector<size_t>> node_children_{}; // indexed by node
        std::vector<glm::mat4> node_transform_{};          // indexed by node, calculated

        std::vector<std::vector<size_t>> roots_;           // indexed by scene
    };
}; // namespace fi

#endif // ENGINE_SCENE_HPP
