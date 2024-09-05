#include "resources/gltf_structure.hpp"

fi::res::gltf_structure::gltf_structure(const gltf_file& file)
{
    nodes_.reserve(file.asset_->nodes.size());
    seq_mapping_.reserve(file.asset_->nodes.size());
    mesh_nodes_.resize(file.meshes_.size());
    weights_.resize(mesh_nodes_.size());

    for (const auto& node : file.asset_->nodes)
    {

        gfx::node_trs& g_node = nodes_.emplace_back();
        if (std::holds_alternative<fgltf::math::fmat4x4>(node.transform))
        {
            memcpy(&g_node.preset_, std::get<fgltf::math::fmat4x4>(node.transform).data(),
                   std::min(sizeof(g_node.preset_), sizeof(fgltf::math::fmat4x4)));
        }
        else
        {
            const auto& trs = std::get<fgltf::TRS>(node.transform);
            g_node.t_ = glm::translate(glm::vec3(trs.translation.x(), trs.translation.y(), trs.translation.z()));
            g_node.r_ = glm::mat4(glm::quat(trs.rotation.w(), trs.scale.x(), trs.scale.y(), trs.scale.z()));
            g_node.s_ = glm::scale(glm::vec3(trs.scale.x(), trs.scale.y(), trs.scale.z()));
        }
        g_node.name_ = node.name;

        if (node.meshIndex)
        {
            mesh_nodes_[*node.meshIndex] = nodes_.size() - 1;
            if (file.asset_->meshes[*node.meshIndex].weights.empty())
            {
                weights_[*node.meshIndex].insert(weights_[*node.meshIndex].end(), //
                                                 node.weights.begin(), node.weights.end());
            }
            else
            {
                weights_[*node.meshIndex].insert(weights_[*node.meshIndex].end(),                      //
                                                 file.asset_->meshes[*node.meshIndex].weights.begin(), //
                                                 file.asset_->meshes[*node.meshIndex].weights.end());
            }
        }
    }

    children_.resize(nodes_.size());
    for (size_t n = 0; n < nodes_.size(); n++)
    {
        children_[n].insert(children_[n].end(),                     //
                            file.asset_->nodes[n].children.begin(), //
                            file.asset_->nodes[n].children.end());
    }

    std::vector<std::vector<uint32_t>> node_layers;
    std::function<void(uint32_t, uint32_t)> build_layer = [&](uint32_t curr, uint32_t depth)
    {
        if (node_layers.size() <= depth)
        {
            node_layers.resize(depth + 1);
        }

        node_layers[depth].push_back(curr);
        for (auto child : children_[curr])
        {
            build_layer(child, depth + 1);
        }
    };

    for (auto root : file.asset_->scenes[0].nodeIndices)
    {
        build_layer(root, 0);
    }

    for (const auto& layer : node_layers)
    {
        seq_mapping_.insert(seq_mapping_.end(), layer.begin(), layer.end());
    }
}

fi::res::gltf_skins::gltf_skins(const gltf_file& file)
{
    mesh_skin_idxs_.resize(file.meshes_.size(), -1);

    for (const auto& node : file.asset_->nodes)
    {
        if (node.skinIndex && node.meshIndex)
        {
            mesh_skin_idxs_[*node.meshIndex] = *node.skinIndex;

            if (skins_.size() <= *node.skinIndex)
            {
                const auto& skin = file.asset_->skins[*node.skinIndex];
                skins_.resize(*node.skinIndex + 1);
                skins_.back().insert(skins_.back().end(), skin.joints.begin(), skin.joints.end());
                inv_binds_.resize(*node.skinIndex + 1);
                inv_binds_.back().reserve(skin.joints.size());

                if (skin.inverseBindMatrices)
                {
                    fgltf::iterateAccessor<glm::mat4>(*file.asset_.get(),
                                                      file.asset_->accessors[*skin.inverseBindMatrices], //
                                                      [&](const glm::mat4& mat) { inv_binds_.back().push_back(mat); });
                }
                else
                {
                    inv_binds_[*node.skinIndex].resize(skin.joints.size(), glm::identity<glm::mat4>());
                }
            }
        }
    }
}