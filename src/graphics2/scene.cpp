#include "graphics2/scene.hpp"

void fi::ResSceneDetails::update_scene_helper(const std::function<void(ResSceneNode& node, size_t node_idx)>& func,
                                              size_t curr, const glm::mat4& parent_transform)
{
    func(nodes_[curr], curr);
    glm::mat4 translation = glm::translate(nodes_[curr].translation_);
    glm::mat4 rotation(nodes_[curr].rotation_);
    glm::mat4 scale = glm::scale(nodes_[curr].scale_);
    node_transform_[curr] = parent_transform * translation * rotation * scale * nodes_[curr].transform_;

    for (auto child_idx : node_children_[curr])
    {
        update_scene_helper(func, child_idx, node_transform_[curr]);
    }
}

void fi::ResSceneDetails::update_scene_helper(size_t curr, const glm::mat4& parent_transform)
{
    glm::mat4 translation = glm::translate(nodes_[curr].translation_);
    glm::mat4 rotation(nodes_[curr].rotation_);
    glm::mat4 scale = glm::scale(nodes_[curr].scale_);
    node_transform_[curr] = parent_transform * translation * rotation * scale * nodes_[curr].transform_;

    for (auto child_idx : node_children_[curr])
    {
        update_scene_helper(child_idx, node_transform_[curr]);
    }
}

fi::ResSceneDetails::ResSceneDetails(const ResDetails& res_details)
{
    const gltf::Model& model = res_details.model();

    std::future<void> node_async = std::async(
        [&]()
        {
            nodes_.reserve(model.nodes.size());
            for (auto& node_in : model.nodes)
            {
                nodes_.emplace_back().name_ = node_in.name;
            }
        });

    std::future<void> children_async = std::async(
        [&]()
        {
            node_children_.reserve(model.nodes.size());
            for (auto& node_in : model.nodes)
            {
                std::vector<size_t>& node_children = node_children_.emplace_back();
                for (auto& child_in : node_in.children)
                {
                    node_children.push_back(child_in);
                }
            }
        });

    std::future<void> transform_async = std::async(
        [&]()
        {
            node_transform_idx_.resize(res_details.material_idxs_.size(), -1);
            node_transform_.resize(model.nodes.size(), glm::identity<glm::mat4>());
            for (size_t n = 0; n < model.nodes.size(); n++)
            {
                if (model.nodes[n].mesh != -1)
                {
                    const ResMesh& mesh = res_details.meshes_[model.nodes[n].mesh];
                    for (size_t p = 0; p < mesh.primitive_count_; p++)
                    {
                        node_transform_idx_[mesh.primitive_idx_ + p] = n;
                    }
                }
            }
        });

    name_ = model.scenes[0].name;
    roots_.reserve(model.scenes[0].nodes.size());
    for (auto node_idx : model.scenes[0].nodes)
    {
        roots_.push_back(node_idx);
    }

    node_async.wait();
    children_async.wait();
    transform_async.wait();
}

void fi::ResSceneDetails::update_scene(const std::function<void(ResSceneNode& node, size_t node_idx)>& func)
{
    for (auto root_node_idx : roots_)
    {
        update_scene_helper(func, root_node_idx, glm::identity<glm::mat4>());
    }
}
void fi::ResSceneDetails::update_scene()
{
    for (auto root_node_idx : roots_)
    {
        update_scene_helper(root_node_idx, glm::identity<glm::mat4>());
    }
}
