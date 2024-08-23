#include "engine/resource.hpp"

fi::SceneResources::SceneResources(const std::filesystem::path& res_path, uint32_t max_instances)
{
    std::filesystem::directory_iterator file_iter(res_path);
    make_unique2(res_detail_);
    for (const auto& file : file_iter)
    {
        res_detail_->add_gltf_file(file.path());
    }
    instancing_infos_.resize(res_detail_->primitives_.size());
    instancing_matrices_.resize(max_instances + instancing_infos_.size(), glm::identity<glm::mat4>());
    bounding_radius_.resize(instancing_infos_.size(), 0);
    renderables_.reserve(res_detail_->primitives_.size());

    for (size_t i = 0; i < instancing_infos_.size(); i++)
    {
        SceneRenderable& renderable = renderables_.emplace_back();
        renderable.instancing_info_ = &instancing_infos_[i];
        renderable.bounding_radius_ = &bounding_radius_[i];

        SceneRenderableRef& renderable_ref = renderable.refs_.emplace_back();
        renderable_ref.matrix_ = &instancing_matrices_[i];
    }

    make_unique2(res_structure_, *res_detail_);
    make_unique2(res_skin_, *res_detail_, *res_structure_);
    for (size_t g = 0; g < res_detail_->gltf_.size(); g++)
    {
        res_anims_.push_back(get_res_animations(*res_detail_, *res_structure_, g));
    }
    res_detail_->lock_and_load();

    vk::DeviceSize padding = sizeof_arr(instancing_infos_) % 16;
    padding = padding ? 16 - padding : 0;

    while (sizeof_arr(bounding_radius_) % 16)
    {
        bounding_radius_.push_back(EMPTY);
    }

    make_unique2(buffer_,sizeof_arr(instancing_infos_) + padding + sizeof_arr(instancing_matrices_));
    buffer_->instancing_infos_ = 0;
    buffer_->instancing_matrices_ = sizeof_arr(instancing_infos_) + padding;
    buffer_->bounding_radius_ = buffer_->instancing_matrices_ + sizeof_arr(instancing_matrices_);
    update_data();

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {};
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
}

fi::SceneResources::~SceneResources()
{
    device().destroyDescriptorSetLayout(set_layout_);
}

void fi::SceneResources::update_data()
{
    memcpy(buffer_->mapping() + buffer_->instancing_infos_, instancing_infos_.data(), sizeof_arr(instancing_infos_));
    memcpy(buffer_->mapping() + buffer_->instancing_matrices_, instancing_matrices_.data(),
           sizeof_arr(instancing_matrices_));
    memcpy(buffer_->mapping() + buffer_->bounding_radius_, bounding_radius_.data(), sizeof_arr(bounding_radius_));
}

void fi::SceneResources::allocate_descriptor(vk::DescriptorPool des_pool)
{
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = des_pool;
    alloc_info.setSetLayouts(set_layout_);
    des_set_ = device().allocateDescriptorSets(alloc_info)[0];

    vk::DeviceSize padding = sizeof_arr(instancing_infos_) % 16;
    padding = padding ? 16 - padding : 0;

    std::array<vk::DescriptorBufferInfo, 3> buffer_infos{};
    buffer_infos[0].buffer = *buffer_;
    buffer_infos[0].offset = buffer_->instancing_infos_;
    buffer_infos[0].range = sizeof_arr(instancing_infos_) + padding;
    buffer_infos[1].buffer = *buffer_;
    buffer_infos[1].offset = buffer_->instancing_matrices_;
    buffer_infos[1].range = sizeof_arr(instancing_matrices_);
    buffer_infos[2].buffer = *buffer_;
    buffer_infos[2].offset = buffer_->bounding_radius_;
    buffer_infos[2].range = sizeof_arr(bounding_radius_);

    vk::WriteDescriptorSet write{};
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.dstBinding = 0;
    write.dstSet = des_set_;
    write.setBufferInfo(buffer_infos);
    device().updateDescriptorSets(write, {});

    res_detail_->allocate_descriptor(des_pool);
    res_structure_->allocate_descriptor(des_pool);
    res_skin_->allocate_descriptor(des_pool);
}

void fi::SceneResources::bind_res(vk::CommandBuffer cmd,
                                  vk::PipelineBindPoint bind_point, //
                                  vk::PipelineLayout pipeline_layout,
                                  uint32_t des_set)
{
    cmd.bindDescriptorSets(bind_point, pipeline_layout, des_set, des_set_, {});
}

void fi::SceneResources::compute(vk::CommandBuffer cmd, const glm::uvec3& work_group)
{
    dispatch_cmd(cmd, work_group);
}
fi::SceneRenderableRef* fi::SceneResources::add_renderable_ref(PrimIdx renderable_idx)
{
    if (ext_instance_count_ + instancing_infos_.size() >= instancing_matrices_.size())
    {
        return nullptr;
    }
    ext_instance_count_++;
    return &renderables_[renderable_idx].refs_.emplace_back(&instancing_matrices_[ext_instance_count_ - 1]);
}
