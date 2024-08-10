#include "graphics2/scene.hpp"

glm::mat4 get_node_transform(const fi::ResSceneNode& node, const glm::mat4& parents)
{
    glm::mat4 translation = glm::translate(node.translation_);
    glm::mat4 rotation(node.rotation_);
    glm::mat4 scale = glm::scale(node.scale_);
    return parents * translation * rotation * scale * node.preset_;
}

void fi::ResSceneDetails::build_scene_layer(size_t curr)
{
    if (node_layers_.size() <= nodes_[curr].depth_)
    {
        node_layers_.emplace_back();
    }
    node_layers_[nodes_[curr].depth_].push_back(curr);

    for (size_t child_idx : node_children_[curr])
    {
        nodes_[child_idx].parent_idx = curr;
        nodes_[child_idx].depth_ = nodes_[curr].depth_ + 1;
        build_scene_layer(child_idx);
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
                node.node_idx = nodes_.size() - 1;
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

    for (size_t root_idx : roots_)
    {
        build_scene_layer(root_idx);
    }

    nodes2_.reserve(nodes_.size());
    nodes2_mapping_.resize(nodes_.size());
    for (const std::vector<size_t>& layer : node_layers_)
    {
        for (size_t node_idx : layer)
        {
            nodes2_.push_back(nodes_[node_idx]);
            nodes2_mapping_[node_idx] = nodes2_.size() - 1;
        }
    }

    free_container_memory(node_children_);
    free_container_memory(node_layers_);
    free_container_memory(roots_);
}

fi::ResSceneDetails::~ResSceneDetails()
{
    device().destroyDescriptorSetLayout(set_layout_);
}

void fi::ResSceneDetails::update_data()
{
    memcpy(buffer_->mapping() + buffer_->node_transform_, node_transform_.data(), sizeof_arr(node_transform_));
}

void fi::ResSceneDetails::reset_scene()
{
    for (ResSceneNode& node : nodes2_)
    {
        node.translation_ = nodes_[node.node_idx].translation_;
        node.rotation_ = nodes_[node.node_idx].rotation_;
        node.scale_ = nodes_[node.node_idx].scale_;
    }
    update_data();
}

void fi::ResSceneDetails::update_scene(const std::function<void(ResSceneNode& node, size_t node_idx)>& func,
                                       const glm::mat4& root_transform)
{
    auto iter = nodes2_.begin();
    while (iter->depth_ == 0 && iter != nodes2_.end())
    {
        func(*iter, iter->node_idx);
        node_transform_[iter->node_idx] = get_node_transform(*iter, root_transform);
        iter++;
    }
    while (iter != nodes2_.end())
    {
        func(*iter, iter->node_idx);
        node_transform_[iter->node_idx] = get_node_transform(*iter, node_transform_[iter->parent_idx]);
        iter++;
    }
    update_data();
}

void fi::ResSceneDetails::update_scene(const glm::mat4& root_transform)
{
    auto iter = nodes2_.begin();
    while (iter->depth_ == 0 && iter != nodes2_.end())
    {
        node_transform_[iter->node_idx] = get_node_transform(*iter, root_transform);
        iter++;
    }
    while (iter != nodes2_.end())
    {
        node_transform_[iter->node_idx] = get_node_transform(*iter, node_transform_[iter->parent_idx]);
        iter++;
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

template <typename OutT>
void set_sampler(const fi::gltf::AnimationSampler& anim_sampler, fi::ResAnimationSampler<OutT>& res_sampler,
                 const fi::gltf::Model& model)
{
    const fi::gltf::Accessor& in_acc = model.accessors[anim_sampler.input];
    const fi::gltf::Accessor& out_acc = model.accessors[anim_sampler.output];
    res_sampler.time_stamps_.reserve(in_acc.count);
    res_sampler.output_.reserve(in_acc.count);

    if (anim_sampler.interpolation == "LINEAR")
    {
        res_sampler.interporlation_method_ = 0;
    }
    else if (anim_sampler.interpolation == "STEP")
    {
        res_sampler.interporlation_method_ = 1;
    }
    else
    {
        res_sampler.interporlation_method_ = 2;
    }

    fi::iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                    { res_sampler.time_stamps_.push_back(*castf(float*, data)); }, in_acc, model);
    fi::iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                    { res_sampler.output_.push_back(*castf(OutT*, data)); }, out_acc, model);
}

std::vector<fi::ResAnimation> fi::load_res_animations(const ResDetails& res_details)
{
    const gltf::Model& model = res_details.model();
    std::vector<ResAnimation> animations;
    animations.resize(model.animations.size());

    if (animations.empty())
    {
        return {};
    }

    std::vector<std::future<void>> animation_asyncs;
    animation_asyncs.reserve(animations.size());
    for (size_t a = 0; a < animations.size(); a++)
    {
        animation_asyncs.emplace_back(std::async(
            [&](const tinygltf::Animation& anim_in, size_t anim_idx)
            {
                ResAnimation& anim = animations[anim_idx];
                anim.key_frames_idx_.resize(model.nodes.size(), -1);
                anim.name_ = anim_in.name;

                for (const auto& channel : anim_in.channels)
                {
                    size_t key_frame_idx = anim.key_frames_idx_[channel.target_node];
                    if (key_frame_idx == -1)
                    {
                        anim.key_frames_.emplace_back();
                        key_frame_idx = anim.key_frames_.size() - 1;
                        anim.key_frames_idx_[channel.target_node] = key_frame_idx;
                    }

                    ResKeyFrames& key_frames = anim.key_frames_[key_frame_idx];
                    const gltf::AnimationSampler& sampler = anim_in.samplers[channel.sampler];
                    if (channel.target_path == "translation")
                    {
                        set_sampler(sampler, key_frames.translation_sampler_, model);
                    }
                    else if (channel.target_path == "rotation")
                    {
                        set_sampler(sampler, key_frames.rotation_sampler_, model);
                    }
                    else if (channel.target_path == "scale")
                    {
                        set_sampler(sampler, key_frames.scale_sampler_, model);
                    }
                    else
                    {
                        continue;
                    }
                }
            },
            model.animations[a], a));
    }

    for (auto& anim_fut : animation_asyncs)
    {
        anim_fut.wait();
    }
    return animations;
}

fi::ResKeyFrame fi::ResKeyFrames::sample_time_stamp(float time_stamp)
{
    return {translation_sampler_.sample_time_stamp(time_stamp, {0, 0, 0}), //
            rotation_sampler_.sample_time_stamp(time_stamp, {0, 0, 0, 1}), //
            scale_sampler_.sample_time_stamp(time_stamp, {1, 1, 1})};
}

void fi::ResKeyFrames::set_sample_time_stamp(float time_stamp, glm::vec3& translation, glm::quat& rotation,
                                             glm::vec3& scale)
{
    ResKeyFrame key_frame = sample_time_stamp(time_stamp);
    translation = key_frame.translation_;
    rotation = key_frame.rotation_;
    scale = key_frame.scale_;
}

void fi::ResKeyFrames::set_sample_time_stamp(float time_stamp, ResSceneNode& node)
{
    ResKeyFrame key_frame = sample_time_stamp(time_stamp);
    node.translation_ = key_frame.translation_;
    node.rotation_ = key_frame.rotation_;
    node.scale_ = key_frame.scale_;
}
