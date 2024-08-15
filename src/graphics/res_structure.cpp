/**
 * @file res_structure.cpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2024-08-15
 * 
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 * 
 */
#include "graphics/res_structure.hpp"

void build_layer(fi::TSNodeIdx node_idx,                                //
                 std::vector<fi::ResStructure::NodeInfo>& tmp_nodes,    //
                 std::vector<std::vector<fi::TSNodeIdx>>& tmp_children, //
                 std::vector<std::vector<fi::TSNodeIdx>>& layers)
{
    if (layers.size() <= tmp_nodes[node_idx].depth_)
    {
        layers.emplace_back();
    }
    layers[tmp_nodes[node_idx].depth_].push_back(node_idx);

    for (size_t child_idx : tmp_children[node_idx])
    {
        tmp_nodes[child_idx].parent_idx_ = node_idx;
        tmp_nodes[child_idx].depth_ = tmp_nodes[node_idx].depth_ + 1;
        build_layer(fi::TSNodeIdx(child_idx), tmp_nodes, tmp_children, layers);
    }
}

inline glm::mat4 get_node_transform(const fi::ResStructure::NodeInfo& node, const glm::mat4& parents)
{
    glm::mat4 translation = glm::translate(node.translation_);
    glm::mat4 rotation(node.rotation_);
    glm::mat4 scale = glm::scale(node.scale_);
    return parents * translation * rotation * scale;
}

fi::ResStructure::ResStructure(ResDetails& res_details)
{
    if (res_details.locked())
    {
        return;
    }

    first_node_.reserve(res_details.gltf_.size());
    for (auto& g : res_details.gltf_)
    {
        old_node_count_ += g.get().nodes.size();
        first_node_.emplace_back(transforms_.size());
        transforms_.resize(old_node_count_, glm::identity<glm::mat4>());
        node_mappings_.resize(transforms_.size());
        nodes_.reserve(transforms_.size());
    }

    bs::thread_pool th_pool;
    std::vector<std::future<void>> futs;
    std::vector<NodeInfo> tmp_nodes;
    std::vector<std::vector<TSNodeIdx>> tmp_children;
    tmp_nodes.reserve(transforms_.size());
    tmp_children.reserve(transforms_.size());

    futs.emplace_back(th_pool.submit_task(
        [&tmp_nodes, &res_details, this]()
        {
            auto first_mesh_iter = res_details.first_mesh_.begin();
            for (const auto& gltf : res_details.gltf_)
            {
                for (const auto& node_in : gltf->nodes)
                {
                    const fgltf::TRS& trs = std::get<0>(node_in.transform);
                    NodeInfo& node = tmp_nodes.emplace_back();
                    node.self_idx_ = tmp_nodes.size() - 1;
                    node.name_ = node_in.name;
                    glms::assign_value(node.translation_, trs.translation, trs.translation.size());
                    glms::assign_value(node.rotation_, trs.rotation, trs.rotation.size());
                    glms::assign_value(node.scale_, trs.scale, trs.scale.size());

                    if (node_in.meshIndex)
                    {
                        MeshInfo& mesh_info = res_details.meshes_[node_in.meshIndex.value() + *first_mesh_iter];
                        mesh_info.node_ = node.self_idx_;

                        const auto& target_weight = gltf->meshes[node_in.meshIndex.value()].weights;
                        if (!target_weight.empty())
                        {
                            mesh_info.morph_weight_ = morph_weight_.size();
                            morph_weight_.reserve(morph_weight_.size() + target_weight.size());
                            for (auto weight : target_weight)
                            {
                                morph_weight_.push_back(weight);
                            }
                        }
                    }
                }
                first_mesh_iter++;
            }
        }));

    futs.emplace_back(th_pool.submit_task(
        [&tmp_children, &res_details, this]()
        {
            auto first_node_iter = first_node_.begin();
            for (const auto& gltf : res_details.gltf_)
            {
                for (const auto& node_in : gltf->nodes)
                {
                    std::vector<TSNodeIdx>& children = tmp_children.emplace_back();
                    children.reserve(node_in.children.size());
                    for (size_t c : node_in.children)
                    {
                        children.emplace_back(c + *first_node_iter);
                    }
                }
                first_node_iter++;
            }
        }));

    for (auto& fut : futs)
    {
        fut.wait();
    }

    std::vector<std::vector<TSNodeIdx>> layers;
    auto first_node_iter = first_node_.begin();
    for (const auto& gltf : res_details.gltf_)
    {
        for (NodeIdx root : gltf->scenes[0].nodeIndices)
        {
            build_layer(TSNodeIdx(root + *first_node_iter), tmp_nodes, tmp_children, layers);
        }
        first_node_iter++;
    }

    for (const auto& layer : layers)
    {
        for (TSNodeIdx node_idx : layer)
        {
            nodes_.push_back(tmp_nodes[node_idx]);
            node_mappings_[node_idx] = TSNodeIdx(nodes_.size() - 1);
        }
    }

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {};
    for (size_t b = 0; b < bindings.size(); b++)
    {
        bindings[b].binding = b;
        bindings[b].descriptorCount = 1;
        bindings[b].descriptorType = vk::DescriptorType::eStorageBuffer;
        bindings[b].stageFlags = vk::ShaderStageFlagBits::eAll;
    }

    layout_info.setBindings(bindings);
    set_layout_ = device().createDescriptorSetLayout(layout_info);
    des_sizes_[0].type = vk::DescriptorType::eStorageBuffer;
    des_sizes_[0].descriptorCount = bindings.size();

    if (morph_weight_.empty())
    {
        morph_weight_.resize(4, std::numeric_limits<float>::min());
    }

    while (sizeof_arr(morph_weight_) % 16)
    {
        morph_weight_.push_back(std::numeric_limits<float>::min());
    }

    make_unique2(buffer_, sizeof_arr(transforms_) + sizeof_arr(morph_weight_));
    buffer_->target_weights_ = sizeof_arr(transforms_);
    buffer_->map_memory();
    update_structure();
}

fi::ResStructure::~ResStructure()
{
    device().destroyDescriptorSetLayout(set_layout_);
}

fi::ResStructure::NodeInfo& fi::ResStructure::index_node(TSNodeIdx idx, size_t gltf_idx)
{
    return nodes_[node_mappings_[first_node_[gltf_idx] + idx]];
}

void fi::ResStructure::update_data(size_t gltf_idx)
{
    vk::DeviceSize offset = 0;
    vk::DeviceSize range = sizeof_arr(transforms_);
    if (gltf_idx != EMPTY)
    {
        offset = sizeof(transforms_[0]) * first_node_[gltf_idx];
        range = first_node_.size() > gltf_idx + 1 //
                    ? sizeof(transforms_[0]) * first_node_[gltf_idx + 1] - offset
                    : range - offset;
    }
    memcpy(buffer_->mapping() + offset, transforms_.data() + offset, range);
    memcpy(buffer_->mapping() + buffer_->target_weights_, morph_weight_.data(), sizeof_arr(morph_weight_));
}

void fi::ResStructure::update_structure(const glm::mat4& transform, size_t gltf_idx)
{
    auto iter = nodes_.begin();
    auto iter_end = nodes_.end();
    if (gltf_idx != EMPTY)
    {
        iter = nodes_.begin() + first_node_[gltf_idx];
        iter_end = first_node_.size() > gltf_idx + 1 ? nodes_.begin() + first_node_[gltf_idx + 1] : nodes_.end();
    }

    while (iter->depth_ == 0 && iter != iter_end)
    {
        transforms_[iter->self_idx_] = get_node_transform(*iter, transform);
        iter++;
    }
    while (iter != iter_end)
    {
        transforms_[iter->self_idx_] = get_node_transform(*iter, transforms_[iter->parent_idx_]);
        iter++;
    }
    update_data(gltf_idx);
}

void fi::ResStructure::allocate_descriptor(vk::DescriptorPool des_pool)
{
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = des_pool;
    alloc_info.setSetLayouts(set_layout_);
    des_set_ = device().allocateDescriptorSets(alloc_info)[0];

    std::array<vk::DescriptorBufferInfo, 2> buffer_infos{};
    buffer_infos[0].buffer = *buffer_;
    buffer_infos[0].offset = buffer_->transforms_;
    buffer_infos[0].range = sizeof_arr(transforms_);
    buffer_infos[1].buffer = *buffer_;
    buffer_infos[1].offset = buffer_->target_weights_;
    buffer_infos[1].range = sizeof_arr(morph_weight_);

    vk::WriteDescriptorSet write{};
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.dstBinding = 0;
    write.dstSet = des_set_;
    write.setBufferInfo(buffer_infos);
    device().updateDescriptorSets(write, {});
}

void fi::ResStructure::bind_res(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set)
{
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, set, des_set_, {});
}
