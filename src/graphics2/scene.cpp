#include "graphics2/scene.hpp"

glm::mat4 get_node_transform(const fi::ResSceneNode& node, const glm::mat4& parents)
{
    glm::mat4 translation = glm::translate(node.translation_);
    glm::mat4 rotation(node.rotation_);
    glm::mat4 scale = glm::scale(node.scale_);
    return parents * translation * rotation * scale;
}

void fi::ResSceneDetails::update_scene_helper(const std::function<void(ResSceneNode& node, size_t node_idx)>& func,
                                              size_t curr, const glm::mat4& parents)
{
    node_transform_[curr] = get_node_transform(nodes_[curr], parents);
    func(nodes_[curr], curr);
    for (auto child_idx : node_children_[curr])
    {
        update_scene_helper(func, child_idx, node_transform_[curr]);
    }
}

void fi::ResSceneDetails::update_scene_helper(size_t curr, const glm::mat4& parents)
{
    node_transform_[curr] = get_node_transform(nodes_[curr], parents);
    for (auto child_idx : node_children_[curr])
    {
        update_scene_helper(child_idx, node_transform_[curr]);
    }
}

fi::ResSceneDetails::ResSceneDetails(const ResDetails& res_details)
{
    const gltf::Model& model = res_details.model();
    name_ = model.scenes[0].name;
    roots_.reserve(model.scenes[0].nodes.size());
    for (auto node_idx : model.scenes[0].nodes)
    {
        roots_.push_back(node_idx);
    }

    std::future<void> node_async = std::async(
        [&]()
        {
            nodes_.reserve(model.nodes.size());
            for (auto& node_in : model.nodes)
            {
                ResSceneNode& node = nodes_.emplace_back();
                node.name_ = node_in.name;
                glms::assign_value(node.translation_, node_in.translation, node_in.translation.size());
                glms::assign_value(node.rotation_, node_in.rotation, node_in.rotation.size());
                glms::assign_value(node.scale_, node_in.scale, node_in.scale.size());
                for (int i = 0; i < node_in.matrix.size(); i += 4)
                {
                    glms::assign_value(node.preset_[i / 4], node_in.matrix.data() + i, 4);
                }
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

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {};
    for (size_t b = 0; b < bindings.size(); b++)
    {
        bindings[b].binding = b;
        bindings[b].descriptorCount = 1;
        bindings[b].descriptorType = vk::DescriptorType::eStorageBuffer;
        bindings[b].stageFlags = vk::ShaderStageFlagBits::eVertex;
    }

    layout_info.setBindings(bindings);
    set_layout_ = device().createDescriptorSetLayout(layout_info);

    des_sizes_[0].type = vk::DescriptorType::eStorageBuffer;
    des_sizes_[0].descriptorCount = bindings.size();

    transform_async.wait();
    make_unique2(buffer_, sizeof_arr(node_transform_idx_) + sizeof_arr(node_transform_));
    buffer_->node_transform_ = sizeof_arr(node_transform_idx_);
    memcpy(buffer_->map_memory(), node_transform_idx_.data(), sizeof_arr(node_transform_idx_));
    memcpy(buffer_->mapping() + buffer_->node_transform_, node_transform_.data(), sizeof_arr(node_transform_));

    node_async.wait();
    children_async.wait();
}

fi::ResSceneDetails::~ResSceneDetails()
{
    device().destroyDescriptorSetLayout(set_layout_);
}

void fi::ResSceneDetails::update_data()
{
    memcpy(buffer_->mapping() + buffer_->node_transform_, node_transform_.data(), sizeof_arr(node_transform_));
}

void fi::ResSceneDetails::update_scene(const std::function<void(ResSceneNode& node, size_t node_idx)>& func,
                                       const glm::mat4& root_transform)
{
    for (auto root_node_idx : roots_)
    {
        update_scene_helper(func, root_node_idx, root_transform);
    }
    update_data();
}
void fi::ResSceneDetails::update_scene(const glm::mat4& root_transform)
{
    for (auto root_node_idx : roots_)
    {
        update_scene_helper(root_node_idx, root_transform);
    }
    update_data();
}

void fi::ResSceneDetails::allocate_descriptor(vk::DescriptorPool des_pool)
{
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = des_pool;
    alloc_info.setSetLayouts(set_layout_);
    des_set_ = device().allocateDescriptorSets(alloc_info)[0];

    std::array<vk::DescriptorBufferInfo, 2> buffer_infos{};
    buffer_infos[0].buffer = *buffer_;
    buffer_infos[0].offset = buffer_->node_transform_idx_;
    buffer_infos[0].range = sizeof_arr(node_transform_idx_);
    buffer_infos[1].buffer = *buffer_;
    buffer_infos[1].offset = buffer_->node_transform_;
    buffer_infos[1].range = sizeof_arr(node_transform_);

    vk::WriteDescriptorSet write{};
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.dstBinding = 0;
    write.dstSet = des_set_;
    write.setBufferInfo(buffer_infos);
    device().updateDescriptorSets(write, {});
}

void fi::ResSceneDetails::bind(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set)
{
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, set, des_set_, {});
}
