#include "graphics/res_holder.hpp"

#include <memory>

std::array<vk::DescriptorPoolSize, 1> fi::ResSceneDetails::des_sizes_ = //
    {vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1}};

void fi::ResSceneDetails::build_scene_layer(SceneNodeIdx curr,                                      //
                                            std::vector<ResSceneNode>& loaded_nodes,                //
                                            const std::vector<std::vector<SceneNodeIdx>>& children, //
                                            std::vector<std::vector<SceneNodeIdx>>& layers)
{
    if (layers.size() <= loaded_nodes[curr].depth_)
    {
        layers.emplace_back();
    }
    layers[loaded_nodes[curr].depth_].push_back(curr);

    for (SceneNodeIdx child_idx : children[curr])
    {
        loaded_nodes[child_idx].parent_idx = curr;
        loaded_nodes[child_idx].depth_ = loaded_nodes[curr].depth_ + 1;
        build_scene_layer(child_idx, loaded_nodes, children, layers);
    }
}

void fi::ResSceneDetails::add_gltf(const std::filesystem::path& path)
{
    if (buffer_)
    {
        return;
    }

    gltf_loader().SetImagesAsIs(false);
    gltf_loader().SetPreserveImageChannels(false);

    // load file
    gltf::Model& model = *models_.emplace_back(std::make_unique<gltf::Model>());
    std::string err = "";
    std::string warnning = "";
    bool loaded = false;
    if (path.extension() == ".glb")
    {
        loaded = gltf_loader().LoadBinaryFromFile(&model, &err, &warnning, path.generic_string());
    }
    else if (path.extension() == ".gltf")
    {
        loaded = gltf_loader().LoadASCIIFromFile(&model, &err, &warnning, path.generic_string());
    }

    if (!loaded)
    {
        throw std::runtime_error(
            std::format("Fail to load {}\nErrors: {}\nWarnning: {}\n", path.generic_string(), err, warnning));
        return;
    }

    // load into temp memories
    first_gltf_node_.push_back(nodes_.size());
    gltf_node_count_.push_back(model.nodes.size());
    std::vector<ResSceneNode> loaded_nodes;
    std::vector<std::vector<SceneNodeIdx>> childrens;

    std::future<void> load_nodes = std::async(
        [&]()
        {
            loaded_nodes.reserve(model.nodes.size());
            for (const gltf::Node& node_in : model.nodes)
            {
                ResSceneNode& node = loaded_nodes.emplace_back();
                node.name_ = node_in.name;
                node.node_idx = loaded_nodes.size() - 1;
                glms::assign_value(node.translation_, node_in.translation, node_in.translation.size());
                glms::assign_value(node.rotation_, node_in.rotation, node_in.rotation.size());
                glms::assign_value(node.scale_, node_in.scale, node_in.scale.size());
                for (int i = 0; i < node_in.matrix.size(); i += 4)
                {
                    glms::assign_value(node.preset_[i / 4], node_in.matrix.data() + i, 4);
                }
            }
        });
    std::future<void> load_children = std::async(
        [&]()
        {
            childrens.reserve(model.nodes.size());
            node_transforms_.resize(node_transforms_.size() + model.nodes.size(), glm::identity<glm::mat4>());
            for (auto& node_in : model.nodes)
            {
                std::vector<SceneNodeIdx>& node_children = childrens.emplace_back();
                for (auto& child_in : node_in.children)
                {
                    node_children.push_back(child_in);
                }
            }
        });

    load_nodes.wait();
    load_children.wait();

    // calculate update orders
    std::vector<std::vector<SceneNodeIdx>> layers;
    nodes_mapping_.resize(nodes_mapping_.size() + loaded_nodes.size());
    nodes_.reserve(nodes_mapping_.size());
    for (SceneNodeIdx root_idx : model.scenes[0].nodes)
    {
        build_scene_layer(root_idx, loaded_nodes, childrens, layers);
    }
    for (const std::vector<SceneNodeIdx>& layer : layers)
    {
        for (SceneNodeIdx node_idx : layer)
        {
            if (loaded_nodes[node_idx].parent_idx != -1)
            {
                loaded_nodes[node_idx].parent_idx += first_gltf_node_.back();
            }
            loaded_nodes[node_idx].node_idx += first_gltf_node_.back();
            nodes_mapping_[loaded_nodes[node_idx].node_idx] = nodes_.size();
            nodes_.push_back(loaded_nodes[node_idx]);
        }
    }
}

fi::ResSceneDetails::~ResSceneDetails()
{
    if (set_layout_)
    {
        device().destroyDescriptorSetLayout(set_layout_);
    }
}

void fi::ResSceneDetails::update_data(size_t gltf_idx)
{
    memcpy(buffer_->mapping() + first_gltf_node_[gltf_idx] * sizeof(glm::mat4),      //
           node_transforms_.data() + first_gltf_node_[gltf_idx] * sizeof(glm::mat4), //
           node_size(gltf_idx) * sizeof(glm::mat4));
}

fi::ResSceneNode& fi::ResSceneDetails::index_node(SceneNodeIdx node_idx, size_t gltf_idx)
{
    return nodes_[nodes_mapping_[first_gltf_node_[gltf_idx] + node_idx]];
}

uint32_t fi::ResSceneDetails::node_size(size_t gltf_idx)
{
    return gltf_node_count_[gltf_idx];
}

void fi::ResSceneDetails::update_gltf(size_t gltf_idx, const glm::mat4& root_transform)
{
    auto get_node_transform = [](const fi::ResSceneNode& node, const glm::mat4& parents)
    {
        glm::mat4 translation = glm::translate(node.translation_);
        glm::mat4 rotation(node.rotation_);
        glm::mat4 scale = glm::scale(node.scale_);
        return parents * translation * rotation * scale * node.preset_;
    };

    auto iter = nodes_.begin() + node_size(gltf_idx);
    auto iter_end = nodes_.begin() + node_size(gltf_idx);
    while (iter->depth_ == 0 && iter != iter_end)
    {
        node_transforms_[iter->node_idx] = get_node_transform(*iter, root_transform);
        iter++;
    }
    while (iter != iter_end)
    {
        node_transforms_[iter->node_idx] = get_node_transform(*iter, node_transforms_[iter->parent_idx]);
        iter++;
    }
    update_data(gltf_idx);
}

void fi::ResSceneDetails::allocate_gpu_res(vk::DescriptorPool des_pool)
{
    make_unique2(buffer_, sizeof_arr(node_transforms_));
    memcpy(buffer_->map_memory(), node_transforms_.data(), sizeof_arr(node_transforms_));

    // create descriptor details
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = vk::DescriptorType::eStorageBuffer;
    bindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    layout_info.setBindings(bindings);
    set_layout_ = device().createDescriptorSetLayout(layout_info);

    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = des_pool;
    alloc_info.setSetLayouts(set_layout_);
    des_set_ = device().allocateDescriptorSets(alloc_info)[0];

    std::array<vk::DescriptorBufferInfo, 1> buffer_infos{};
    buffer_infos[0].buffer = *buffer_;
    buffer_infos[0].offset = 0;
    buffer_infos[0].range = sizeof_arr(node_transforms_);

    vk::WriteDescriptorSet write{};
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.dstBinding = 0;
    write.dstSet = des_set_;
    write.setBufferInfo(buffer_infos);
    device().updateDescriptorSets(write, {});

    for (int i = 0; i < gltf_node_count_.size(); i++)
    {
        update_gltf(i);
    }
}

const std::array<vk::DescriptorPoolSize, 1>& fi::ResSceneDetails::descriptor_pool_sizes() const
{
    return des_sizes_;
}

const std::vector<std::unique_ptr<gltf::Model>>& fi::ResSceneDetails::models() const { return models_; }

void fi::ResSceneDetails::bind(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set)
{
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, set, des_set_, {});
}
