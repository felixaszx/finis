#include "engine/particle.hpp"

fi::ParticleGroup::ParticleGroup(uint32_t max_particles, uint32_t max_targets)
{
    particles_.resize(max_particles);
    targets_.resize(max_targets);

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {};
    for (size_t b = 0; b < bindings.size(); b++)
    {
        bindings[b].binding = b;
        bindings[b].descriptorCount = 1;
        bindings[b].descriptorType = vk::DescriptorType::eStorageBuffer;
        bindings[b].stageFlags = vk::ShaderStageFlagBits::eAll;
    }
    bindings[2].descriptorType = vk::DescriptorType::eUniformBuffer;

    layout_info.setBindings(bindings);
    set_layout_ = device().createDescriptorSetLayout(layout_info);
    des_sizes_[0].type = vk::DescriptorType::eStorageBuffer;
    des_sizes_[0].descriptorCount = 2;
    des_sizes_[1].type = vk::DescriptorType::eUniformBuffer;
    des_sizes_[1].descriptorCount = 1;
}

fi::ParticleGroup::~ParticleGroup()
{
    device().destroyDescriptorSetLayout(set_layout_);
}

void fi::ParticleGroup::lock_and_load()
{
    if (locked_)
    {
        return;
    }
    locked_ = true;

    make_unique2(device_buffer_, sizeof_arr(particles_), DST);
    make_unique2(host_buffer_, sizeof_arr(targets_) * sizeof(uniform_data));
    Buffer<BufferBase::EmptyExtraInfo, vertex, seq_write, host_cached, hosted> staging(device_buffer_->size(), SRC);
    memcpy(staging.map_memory(), particles_.data(), sizeof_arr(particles_));

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    cmd.copyBuffer(staging, *device_buffer_, {0, 0, VK_WHOLE_SIZE});
    cmd.end();
    submit_one_time_cmd(cmd);
    update_buffer();
}

void fi::ParticleGroup::update_buffer()
{
    memcpy(host_buffer_->mapping() + host_buffer_->targets_, targets_.data(),
           sizeof(targets_[0]) * uniform_data.target_count_);
    memcpy(host_buffer_->mapping() + host_buffer_->uniforms_, &uniform_data, sizeof(uniform_data));
}

void fi::ParticleGroup::bind_res(vk::CommandBuffer cmd, vk::PipelineBindPoint bind_point, //
                                 vk::PipelineLayout pipeline_layout, uint32_t des_set)
{

    cmd.bindDescriptorSets(bind_point, pipeline_layout, des_set, des_set_, {});
}

void fi::ParticleGroup::compute(vk::CommandBuffer cmd, const glm::vec3& work_group)
{
    dispatch_cmd(cmd, work_group);
}
