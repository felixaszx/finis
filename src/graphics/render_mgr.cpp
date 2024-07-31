#include "graphics/graphics.hpp"
#include "graphics/render_mgr.hpp"
#include "graphics/animation_mgr.hpp"

fi::RenderMgr::~RenderMgr()
{
    if (pool_)
    {
        device().destroyDescriptorPool(pool_);
    }

    for (auto set_layout : texture_set_layouts_)
    {
        device().destroyDescriptorSetLayout(set_layout);
    }

    for (auto sampler : sampelers_)
    {
        device().destroySampler(sampler);
    }
}

std::vector<vk::DescriptorSetLayout> fi::RenderMgr::texture_set_layouts() const
{
    return locked_ ? texture_set_layouts_ : std::vector<vk::DescriptorSetLayout>();
}

std::pair<fi::RenderMgr::DataIdx, size_t> fi::RenderMgr::upload_res(const std::filesystem::path& path,
                                                                    TextureMgr& texture_mgr,
                                                                    AnimationMgr& animation_mgr,
                                                                    gltf::Expected<gltf::GltfDataBuffer>& gltf_file)
{
    if (locked_)
    {
        return {};
    }
    gltf_file = gltf::GltfDataBuffer::FromPath(path);
    auto asset = parser_.loadGltf(gltf_file.get(), path.parent_path(),
                                  gltf::Options::GenerateMeshIndices | //
                                      gltf::Options::LoadExternalBuffers);

    std::vector<vk::Sampler> samplers;
    std::vector<vk::DescriptorImageInfo>& texture_infos = texture_infos_.emplace_back();
    samplers.reserve(asset->samplers.size());
    texture_infos.reserve(asset->textures.size());

    auto check_mipmap = [](gltf::Filter filter)
    {
        switch (filter)
        {
            case gltf::Filter::Nearest:
            case gltf::Filter::Linear:
                return false;
            default:
                return true;
        }
    };

    for (auto& sampler : asset->samplers)
    {
        auto decode_adress_mode = [&](gltf::Wrap wrap)
        {
            switch (wrap)
            {
                default:
                case gltf::Wrap::Repeat:
                    return vk::SamplerAddressMode::eRepeat;
                case gltf::Wrap::MirroredRepeat:
                    return vk::SamplerAddressMode::eMirroredRepeat;
                case gltf::Wrap::ClampToEdge:
                    return vk::SamplerAddressMode::eClampToEdge;
            }
        };

        auto extract_filter = [](gltf::Filter filter)
        {
            switch (filter)
            {
                case gltf::Filter::Nearest:
                case gltf::Filter::NearestMipMapNearest:
                case gltf::Filter::NearestMipMapLinear:
                    return vk::Filter::eNearest;
                case gltf::Filter::Linear:
                case gltf::Filter::LinearMipMapNearest:
                case gltf::Filter::LinearMipMapLinear:
                default:
                    return vk::Filter::eLinear;
            }
        };

        auto extract_mipmap_mode = [](gltf::Filter filter)
        {
            switch (filter)
            {
                case gltf::Filter::NearestMipMapNearest:
                case gltf::Filter::LinearMipMapNearest:
                    return vk::SamplerMipmapMode::eNearest;
                case gltf::Filter::NearestMipMapLinear:
                case gltf::Filter::LinearMipMapLinear:
                default:
                    return vk::SamplerMipmapMode::eLinear;
            }
        };

        vk::SamplerCreateInfo sampler_info{};
        sampler_info.anisotropyEnable = true;
        sampler_info.maxAnisotropy = 4;
        sampler_info.addressModeU = decode_adress_mode(sampler.wrapS);
        sampler_info.addressModeV = decode_adress_mode(sampler.wrapT);
        sampler_info.borderColor = vk::BorderColor::eFloatOpaqueBlack;
        sampler_info.maxLod = check_mipmap(sampler.minFilter.value_or(gltf::Filter::Linear)) ? 1000.0f : 0.0f;
        sampler_info.mipmapMode = extract_mipmap_mode(sampler.magFilter.value_or(gltf::Filter::LinearMipMapLinear));
        sampler_info.magFilter = extract_filter(sampler.magFilter.value_or(gltf::Filter::Linear));
        sampler_info.minFilter = extract_filter(sampler.minFilter.value_or(gltf::Filter::Linear));
        samplers.push_back(device().createSampler(sampler_info));
    }
    sampelers_.insert(sampelers_.end(), samplers.begin(), samplers.end());

    for (auto& tex : asset->textures)
    {
        size_t img_idx = tex.imageIndex.value();
        std::string tex_path = path.parent_path().generic_string() + '/' +
                               std::get<gltf::sources::URI>(asset->images[img_idx].data).uri.c_str();
        Texture& tex_info = texture_mgr.load_texture(
            tex_path,
            check_mipmap(asset->samplers[tex.samplerIndex.value_or(0)].minFilter.value_or(gltf::Filter::Linear)));
        tex_info.sampler_ = samplers[tex.samplerIndex.value_or(0)];
        texture_infos.push_back(tex_info);
    }

    std::vector<Material>& materials = materials_.emplace_back();
    materials.reserve(asset->materials.size());
    for (auto& a_mat : asset->materials)
    {
        Material& p_mat = materials.emplace_back();
        // basic pbr texture
        glms::assign_value(p_mat.color_factor_, a_mat.pbrData.baseColorFactor);
        p_mat.color_texture_idx_ = a_mat.pbrData.baseColorTexture //
                                       ? a_mat.pbrData.baseColorTexture->textureIndex
                                       : 0;
        p_mat.metalic_ = a_mat.pbrData.metallicFactor;
        p_mat.roughtness_ = a_mat.pbrData.roughnessFactor;
        p_mat.metalic_roughtness_ = a_mat.pbrData.metallicRoughnessTexture //
                                        ? a_mat.pbrData.metallicRoughnessTexture->textureIndex
                                        : ~0;

        // occlusion map
        if (a_mat.occlusionTexture)
        {
            p_mat.occlusion_map_idx_ = a_mat.occlusionTexture->textureIndex;
        }

        // aplha value
        p_mat.alpha_value_ = a_mat.alphaCutoff;

        // normal
        if (a_mat.normalTexture)
        {
            p_mat.normal_scale_ = a_mat.normalTexture->scale;
            p_mat.normal_map_idx_ = a_mat.normalTexture->textureIndex;
        }

        // emissive
        p_mat.combined_emissive_factor_[3] = a_mat.emissiveStrength;
        glms::assign_value(p_mat.combined_emissive_factor_, a_mat.emissiveFactor, 3);
        if (a_mat.emissiveTexture)
        {
            p_mat.emissive_map_idx_ = a_mat.emissiveTexture->textureIndex;
        }

        // anistropy
        if (a_mat.anisotropy)
        {
            p_mat.anistropy_rotation_ = a_mat.anisotropy->anisotropyRotation;
            p_mat.anistropy_strength_ = a_mat.anisotropy->anisotropyStrength;
            if (a_mat.anisotropy->anisotropyTexture)
            {
                p_mat.anistropy_map_idx_ = a_mat.anisotropy->anisotropyTexture->textureIndex;
            }
        }

        // specular
        if (a_mat.specular)
        {
            p_mat.combined_spec_factor_[3] = a_mat.specular->specularFactor;
            glms::assign_value(p_mat.combined_spec_factor_, a_mat.specular->specularColorFactor, 3);
            if (a_mat.specular->specularColorTexture)
            {
                p_mat.spec_color_map_idx_ = a_mat.specular->specularColorTexture->textureIndex;
            }
            if (a_mat.specular->specularTexture)
            {
                p_mat.spec_map_idx_ = a_mat.specular->specularTexture->textureIndex;
            }
        }

        // transmission
        if (a_mat.transmission)
        {
            p_mat.transmission_factor_ = a_mat.transmission->transmissionFactor;
            if (a_mat.transmission->transmissionTexture)
            {
                p_mat.transmission_map_idx_ = a_mat.transmission->transmissionTexture->textureIndex;
            }
        }

        // volume
        if (a_mat.volume)
        {
            p_mat.combined_attenuation_[3] = a_mat.volume->attenuationDistance;
            glms::assign_value(p_mat.combined_attenuation_, a_mat.volume->attenuationColor, 3);
            p_mat.thickness_factor_ = a_mat.volume->thicknessFactor;
            if (a_mat.volume->thicknessTexture)
            {
                p_mat.thickness_map_idx_ = a_mat.volume->thicknessTexture->textureIndex;
            }
        }

        // sheen
        if (a_mat.sheen)
        {
            p_mat.combined_sheen_color_factor_[3] = a_mat.sheen->sheenRoughnessFactor;
            glms::assign_value(p_mat.combined_sheen_color_factor_, a_mat.sheen->sheenColorFactor, 3);
            if (a_mat.sheen->sheenColorTexture)
            {
                p_mat.sheen_color_map_idx_ = a_mat.sheen->sheenColorTexture->textureIndex;
            }
            if (a_mat.sheen->sheenRoughnessTexture)
            {
                p_mat.sheen_roughtness_map_idx_ = a_mat.sheen->sheenRoughnessTexture->textureIndex;
            }
        }
    }

    std::vector<uint32_t>& mat_idxs = mat_idxs_.emplace_back();
    std::vector<vk::DrawIndexedIndirectCommand>& draw_calls = draw_calls_.emplace_back();

    std::vector<Renderable::Vertex> vtxs;
    std::vector<uint32_t> indices;
    size_t old_vtx_count = 0;
    size_t old_idx_size = 0;
    for (auto& mesh : asset->meshes)
    {
        for (auto& p : mesh.primitives)
        {
            gltf::Accessor idx_acs = asset->accessors[p.indicesAccessor.value()];
            indices.reserve(old_idx_size + idx_acs.count);
            gltf::iterateAccessor<std::uint32_t>(asset.get(), idx_acs,
                                                 [&](std::uint32_t idx) { indices.push_back(idx); });

            auto& draw_call = draw_calls.emplace_back();
            draw_call.firstIndex = old_idx_size;
            draw_call.indexCount = idx_acs.count;
            draw_call.vertexOffset = old_vtx_count;
            draw_call.firstInstance = 0;
            draw_call.instanceCount = 1;
            old_idx_size = indices.size();

            gltf::Accessor pos_acs = asset->accessors[p.findAttribute("POSITION")->second];
            vtxs.resize(old_vtx_count + pos_acs.count);
            gltf::iterateAccessorWithIndex<gltf::math::fvec3>(
                asset.get(), pos_acs, [&](const gltf::math::fvec3& vtx, size_t vtx_idx)
                { glms::assign_value(vtxs[old_vtx_count + vtx_idx].position_, vtx); });

            auto normal_iter = p.findAttribute("NORMAL");
            if (normal_iter != p.attributes.end())
            {
                gltf::iterateAccessorWithIndex<gltf::math::fvec3>(
                    asset.get(), asset->accessors[normal_iter->second],
                    [&](const gltf::math::fvec3& normal, size_t vtx_idx)
                    { glms::assign_value(vtxs[old_vtx_count + vtx_idx].normal_, normal); });
            }

            auto tangent_iter = p.findAttribute("TANGENT");
            if (tangent_iter != p.attributes.end())
            {
                gltf::iterateAccessorWithIndex<gltf::math::fvec4>(
                    asset.get(), asset->accessors[tangent_iter->second],
                    [&](const gltf::math::fvec4& tangent, size_t vtx_idx)
                    { glms::assign_value(vtxs[old_vtx_count + vtx_idx].tangent_, tangent); });
            }

            auto tex_coord_iter = p.findAttribute("TEXCOORD_0");
            if (tex_coord_iter != p.attributes.end())
            {
                gltf::iterateAccessorWithIndex<gltf::math::fvec2>(
                    asset.get(), asset->accessors[tex_coord_iter->second],
                    [&](const gltf::math::fvec2& tex_coord, size_t vtx_idx)
                    { glms::assign_value(vtxs[old_vtx_count + vtx_idx].tex_coord_, tex_coord); });
            }

            mat_idxs.push_back(p.materialIndex.value_or(0));
            old_vtx_count = vtxs.size();
            // tbd
        }
    }
    while (mat_idxs.size() % 16)
    {
        mat_idxs.push_back(~0);
    }

    auto& host_buffer = host_buffers_.emplace_back(sizeof_arr(draw_calls) + 0);
    host_buffer.draw_call_offset_ = 0;
    memcpy(host_buffer.map_memory() + host_buffer.draw_call_offset_, draw_calls.data(), sizeof_arr(draw_calls));

    uint32_t mat_buffer_padding = (sizeof_arr(vtxs) + sizeof_arr(indices)) % 16;
    mat_buffer_padding = mat_buffer_padding ? 16 - mat_buffer_padding : 0;
    auto& device_buffer = device_buffers_.emplace_back(sizeof_arr(vtxs)                                 //
                                                           + sizeof_arr(indices)                        //
                                                           + mat_buffer_padding + sizeof_arr(materials) //
                                                           + sizeof_arr(mat_idxs),
                                                       DST);
    Buffer<BufferBase::EmptyExtraInfo, seq_write> staging(device_buffer.size(), SRC);
    staging.map_memory();
    device_buffer.vtx_offset_ = 0;
    device_buffer.idx_offset_ = sizeof_arr(vtxs);
    device_buffer.mat_offset_ = device_buffer.idx_offset_ + sizeof_arr(indices) + mat_buffer_padding;
    device_buffer.mat_idx_offset_ = device_buffer.mat_offset_ + sizeof_arr(materials);

    memcpy(staging.mapping() + device_buffer.vtx_offset_, vtxs.data(), sizeof_arr(vtxs));
    memcpy(staging.mapping() + device_buffer.idx_offset_, indices.data(), sizeof_arr(indices));
    memcpy(staging.mapping() + device_buffer.mat_offset_, materials.data(), sizeof_arr(materials));
    memcpy(staging.mapping() + device_buffer.mat_idx_offset_, mat_idxs.data(), sizeof_arr(mat_idxs));

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    cmd.copyBuffer(staging, device_buffer, {{0, 0, device_buffer.size()}});
    cmd.end();
    submit_one_time_cmd(cmd);
    animation_mgr.upload_res(*this, asset);
    return {device_buffers_.size() - 1, draw_calls.size()};
}

void fi::RenderMgr::draw(const std::vector<DataIdx>& draws,
                         const std::function<void(vk::Buffer device_buffer, //
                                                  uint32_t vtx_buffer_binding,
                                                  const VtxIdxBufferExtra& offsets, //
                                                  vk::Buffer host_buffer,
                                                  const HostBufferExtra& host_offsets, //
                                                  vk::DescriptorSet texture_set)>& draw_func)
{
    if (locked_)
    {
        for (auto data_idx : draws)
        {
            draw_func(device_buffers_[data_idx], 0, device_buffers_[data_idx], host_buffers_[data_idx],
                      host_buffers_[data_idx], texture_sets_[data_idx]);
        }
    }
}

void fi::RenderMgr::lock_and_prepared()
{
    if (locked_)
    {
        return;
    }

    locked_ = true;
    texture_set_layouts_.reserve(texture_infos_.size());

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    bindings[0].stageFlags = vk::ShaderStageFlagBits::eFragment;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = vk::DescriptorType::eStorageBuffer;
    bindings[1].stageFlags = vk::ShaderStageFlagBits::eFragment;

    bindings[2].binding = 2;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType = vk::DescriptorType::eStorageBuffer;
    bindings[2].stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorPoolSize, 2> sizes{};
    sizes[0].type = vk::DescriptorType::eCombinedImageSampler;
    sizes[1].type = vk::DescriptorType::eStorageBuffer;
    sizes[1].descriptorCount = materials_.size() + mat_idxs_.size();
    for (auto& infos : texture_infos_)
    {
        bindings[0].descriptorCount = infos.size();
        sizes[0].descriptorCount += bindings[0].descriptorCount;

        vk::DescriptorSetLayoutCreateInfo layout_info{};
        layout_info.setBindings(bindings);
        texture_set_layouts_.emplace_back(device().createDescriptorSetLayout(layout_info));
    }

    vk::DescriptorPoolCreateInfo pool_info{};
    pool_info.setPoolSizes(sizes);
    pool_info.maxSets = texture_set_layouts_.size();
    pool_ = device().createDescriptorPool(pool_info);

    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = pool_;
    alloc_info.setSetLayouts(texture_set_layouts_);
    texture_sets_ = device().allocateDescriptorSets(alloc_info);

    for (size_t i = 0; i < texture_sets_.size(); i++)
    {
        vk::WriteDescriptorSet write_textures{};
        write_textures.dstSet = texture_sets_[i];
        write_textures.dstBinding = 0;
        write_textures.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        write_textures.setImageInfo(texture_infos_[i]);

        vk::DescriptorBufferInfo ssbo_info{};
        ssbo_info.buffer = device_buffers_[i];
        ssbo_info.offset = device_buffers_[i].mat_offset_;
        ssbo_info.range = sizeof_arr(materials_[i]);
        vk::WriteDescriptorSet write_ssbo{};
        write_ssbo.dstSet = texture_sets_[i];
        write_ssbo.dstBinding = 1;
        write_ssbo.descriptorType = vk::DescriptorType::eStorageBuffer;
        write_ssbo.setBufferInfo(ssbo_info);

        vk::DescriptorBufferInfo ssbo_info1{};
        ssbo_info1.buffer = device_buffers_[i];
        ssbo_info1.offset = device_buffers_[i].mat_idx_offset_;
        ssbo_info1.range = sizeof_arr(mat_idxs_[i]);
        vk::WriteDescriptorSet write_ssbo1{};
        write_ssbo1.dstSet = texture_sets_[i];
        write_ssbo1.dstBinding = 2;
        write_ssbo1.descriptorType = vk::DescriptorType::eStorageBuffer;
        write_ssbo1.setBufferInfo(ssbo_info1);

        device().updateDescriptorSets({write_textures, write_ssbo, write_ssbo1}, {});
    }
}

fi::Renderable fi::RenderMgr::get_renderable(DataIdx data_idx, size_t renderable_idx)
{
    Renderable rr{};
    rr.draw_call_ = &draw_calls_[data_idx][renderable_idx];
    rr.mat_ = &materials_[data_idx][renderable_idx];
    return rr;
}

std::vector<vk::VertexInputBindingDescription> fi::Renderable::vtx_bindings()
{
    std::vector<vk::VertexInputBindingDescription> bindings(1);

    for (size_t i = 0; i < bindings.size(); i++)
    {
        bindings[i].binding = i;
        bindings[i].inputRate = vk::VertexInputRate::eVertex;
        bindings[i].stride = sizeof(Renderable::Vertex);
    }

    return bindings;
}

std::vector<vk::VertexInputAttributeDescription> fi::Renderable::vtx_attributes()
{
    std::vector<vk::VertexInputAttributeDescription> attributes(4);
    for (size_t i = 0; i < attributes.size(); i++)
    {
        attributes[i].binding = 0;
        attributes[i].format = vk::Format::eR32G32B32Sfloat;
        attributes[i].location = i;
    }
    attributes[3].format = vk::Format::eR32G32Sfloat;

    attributes[0].offset = offsetof(Renderable::Vertex, position_);
    attributes[1].offset = offsetof(Renderable::Vertex, normal_);
    attributes[2].offset = offsetof(Renderable::Vertex, tangent_);
    attributes[3].offset = offsetof(Renderable::Vertex, tex_coord_);
    return attributes;
}
