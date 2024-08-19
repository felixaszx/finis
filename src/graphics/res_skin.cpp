/**
 * @file res_skin.cpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#include "graphics/res_skin.hpp"

fi::ResSkinDetails::ResSkinDetails(ResDetails& res_details, ResStructure& res_structure)
{
    if (res_details.locked())
    {
        return;
    }

    for (size_t g = 0; g < res_details.gltf_.size(); g++)
    {
        first_skin_.push_back(skins_.size());
        skins_.reserve(skins_.size() + res_details.gltf_[g]->skins.size());
        for (const auto& skin : res_details.gltf_[g]->skins)
        {
            SkinInfo& skin_info = skins_.emplace_back(skin.name.c_str(), TSNodeIdx(joints_.size()), skin.joints.size());
            joints_.reserve(joints_.size() + skin_info.joint_count_);
            inv_binds_.reserve(inv_binds_.size() + skin_info.joint_count_);
            for (auto joint : skin.joints)
            {
                joints_.emplace_back(res_structure.index_node(TSNodeIdx(joint), g).self_idx_);
            }

            if (skin.inverseBindMatrices)
            {
                const fgltf::Accessor& acc = res_details.gltf_[g]->accessors[skin.inverseBindMatrices.value()];
                fgltf::iterateAccessor<glm::mat4>(res_details.gltf_[g].get(), acc, [&](const glm::mat4& inv_binding)
                                                  { inv_binds_.push_back(inv_binding); });
            }
            else
            {
                inv_binds_.resize(joints_.size(), glm::identity<glm::mat4>());
            }
        }
    }

    for (size_t g = 0; g < res_details.gltf_.size(); g++)
    {
        for (const auto& node : res_details.gltf_[g]->nodes)
        {
            if (node.skinIndex && node.meshIndex)
            {
                TSMeshIdx m(res_details.first_mesh_[g] + node.meshIndex.value());
                res_details.meshes_[m].first_joint = skins_[first_skin_[g] + node.skinIndex.value()].first_joint_;
            }
        }
    }

    if (skins_.empty())
    {
        joints_.resize(4, EMPTY);
        inv_binds_.push_back(glm::zero<glm::mat4>());
        int* a = new int;
    }

    while (sizeof_arr(joints_) % 16)
    {
        joints_.push_back(EMPTY);
    }

    make_unique2(buffer_, sizeof_arr(joints_) + sizeof_arr(inv_binds_), DST);
    buffer_->inv_binds_ = sizeof_arr(joints_);

    Buffer<BufferBase::EmptyExtraInfo, vertex, seq_write, host_cached, hosted> staging(buffer_->size(), SRC);
    memcpy(staging.map_memory(), joints_.data(), sizeof_arr(joints_));
    memcpy(staging.mapping() + buffer_->inv_binds_, inv_binds_.data(), sizeof_arr(inv_binds_));
    staging.flush_cache();
    staging.unmap_memory();

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    cmd.copyBuffer(staging, *buffer_, {{0, 0, buffer_->size()}});
    cmd.end();
    submit_one_time_cmd(cmd);

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
}

fi::ResSkinDetails::~ResSkinDetails()
{
    device().destroyDescriptorSetLayout(set_layout_);
}

void fi::ResSkinDetails::allocate_descriptor(vk::DescriptorPool des_pool)
{
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = des_pool;
    alloc_info.setSetLayouts(set_layout_);
    des_set_ = device().allocateDescriptorSets(alloc_info)[0];

    std::array<vk::DescriptorBufferInfo, 2> buffer_infos{};
    buffer_infos[0].buffer = *buffer_;
    buffer_infos[0].offset = buffer_->joints_;
    buffer_infos[0].range = sizeof_arr(joints_);
    buffer_infos[1].buffer = *buffer_;
    buffer_infos[1].offset = buffer_->inv_binds_;
    buffer_infos[1].range = sizeof_arr(inv_binds_);

    vk::WriteDescriptorSet write{};
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.dstBinding = 0;
    write.dstSet = des_set_;
    write.setBufferInfo(buffer_infos);
    device().updateDescriptorSets(write, {});
}

void fi::ResSkinDetails::bind_res(vk::CommandBuffer cmd, vk::PipelineBindPoint bind_point, //
                                  vk::PipelineLayout pipeline_layout, uint32_t des_set)
{
    cmd.bindDescriptorSets(bind_point, pipeline_layout, des_set, des_set_, {});
}
