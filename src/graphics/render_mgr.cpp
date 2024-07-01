#include "graphics/graphics.hpp"
#include "graphics/render_mgr.hpp"

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
                                                                    TextureMgr& texture_mgr)
{
    if (locked_)
    {
        return {};
    }
    auto gltf_file = gltf::GltfDataBuffer::FromPath(path);
    auto asset = parser_.loadGltf(gltf_file.get(), path.parent_path(),
                                  gltf::Options::LoadExternalImages | //
                                      gltf::Options::GenerateMeshIndices);

    std::vector<vk::Sampler> samplers;
    std::vector<vk::DescriptorImageInfo>& texture_infos = texture_infos_.emplace_back();
    samplers.reserve(asset->samplers.size());
    texture_infos.reserve(asset->textures.size());

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
        sampler_info.maxLod = 1000.0f;
        sampler_info.mipmapMode = extract_mipmap_mode(sampler.magFilter.value_or(gltf::Filter::LinearMipMapLinear));
        sampler_info.magFilter = extract_filter(sampler.magFilter.value_or(gltf::Filter::Linear));
        sampler_info.minFilter = extract_filter(sampler.minFilter.value_or(gltf::Filter::Linear));
        samplers.push_back(device().createSampler(sampler_info));
    }
    sampelers_.insert(sampelers_.end(), samplers.begin(), samplers.end());

    for (auto& tex : asset->textures)
    {
        size_t img_idx = tex.imageIndex.value();
        size_t buffer_view_idx = std::get<gltf::sources::BufferView>(asset->images[img_idx].data).bufferViewIndex;
        gltf::BufferView& view = asset->bufferViews[buffer_view_idx];
        gltf::StaticVector<std::uint8_t>& img_data =
            std::get<gltf::sources::Array>(asset->buffers[view.bufferIndex].data).bytes;

        Texture& tex_info =
            texture_mgr.load_texture(tex.name == "" ? asset->images[img_idx].name.c_str() : tex.name.c_str(), img_data,
                                     view.byteOffset, view.byteLength);
        tex_info.sampler_ = samplers[tex.samplerIndex.value_or(0)];
        texture_infos.push_back(tex_info);
    }

    std::vector<Material>& materials = materials_.emplace_back();
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

            auto tex_coord_iter = p.findAttribute("TEXCOORD_0");
            if (tex_coord_iter != p.attributes.end())
            {
                gltf::iterateAccessorWithIndex<gltf::math::fvec2>(
                    asset.get(), asset->accessors[tex_coord_iter->second],
                    [&](const gltf::math::fvec2& tex_coord, size_t vtx_idx)
                    { glms::assign_value(vtxs[old_vtx_count + vtx_idx].tex_coord_, tex_coord); });
            }

            Material& p_mat = materials.emplace_back();
            auto& a_mat = asset->materials[p.materialIndex.value_or(0)];

            // basic pbr texture
            glms::assign_value(p_mat.color_factor_, a_mat.pbrData.baseColorFactor);
            p_mat.color_texture_idx_ = a_mat.pbrData.baseColorTexture //
                                           ? a_mat.pbrData.baseColorTexture->textureIndex
                                           : 0;
            p_mat.metalic_ = a_mat.pbrData.metallicFactor;
            p_mat.roughtness_ = a_mat.pbrData.roughnessFactor;
            p_mat.metalic_roughtness_ = a_mat.pbrData.metallicRoughnessTexture //
                                            ? a_mat.pbrData.metallicRoughnessTexture->textureIndex
                                            : 0;
            old_vtx_count = vtxs.size();
            // tbd
        }
    }

    vk::DeviceSize mat_buffer_padding = 16 - (sizeof_arr(vtxs) + sizeof_arr(indices)) % 16;
    auto& buffer = device_buffers_.emplace_back(sizeof_arr(vtxs)                                 //
                                                    + sizeof_arr(indices)                        //
                                                    + mat_buffer_padding + sizeof_arr(materials) //
                                                    + sizeof_arr(draw_calls),
                                                DST);
    Buffer<BufferBase::EmptyExtraInfo, seq_write> staging(buffer.size(), SRC);
    staging.map_memory();
    buffer.vtx_offset_ = 0;
    buffer.idx_offset_ = sizeof_arr(vtxs);
    buffer.mat_offset_ = buffer.idx_offset_ + sizeof_arr(indices) + mat_buffer_padding;
    buffer.draw_call_offset_ = buffer.mat_offset_ + sizeof(materials);

    memcpy(staging.mapping() + buffer.vtx_offset_, vtxs.data(), sizeof_arr(vtxs));
    memcpy(staging.mapping() + buffer.idx_offset_, indices.data(), sizeof_arr(indices));
    memcpy(staging.mapping() + buffer.mat_offset_, materials.data(), sizeof_arr(materials));
    memcpy(staging.mapping() + buffer.draw_call_offset_, draw_calls.data(), sizeof_arr(draw_calls));

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    cmd.copyBuffer(staging, buffer, {{0, 0, buffer.size()}});
    cmd.end();
    submit_one_time_cmd(cmd);
    return {device_buffers_.size() - 1, draw_calls.size()};
}

void fi::RenderMgr::draw(const std::vector<DataIdx>& draws,
                         const std::function<void(vk::Buffer device_buffer, const VtxIdxBufferExtra& offsets,
                                                  vk::DescriptorSet texture_set)>& draw_func)
{
    if (!locked_)
    {
        return;
    }

    for (auto data_idx : draws)
    {
        draw_func(device_buffers_[data_idx], device_buffers_[data_idx], texture_sets_[data_idx]);
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

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    bindings[0].stageFlags = vk::ShaderStageFlagBits::eFragment;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = vk::DescriptorType::eStorageBuffer;
    bindings[1].stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorPoolSize, 2> sizes{};
    sizes[0].type = vk::DescriptorType::eCombinedImageSampler;
    sizes[1].type = vk::DescriptorType::eStorageBuffer;
    sizes[1].descriptorCount = materials_.size();
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
        ssbo_info.range = sizeof(materials_[i]);
        vk::WriteDescriptorSet write_ssbo{};
        write_ssbo.dstSet = texture_sets_[i];
        write_ssbo.dstBinding = 1;
        write_ssbo.descriptorType = vk::DescriptorType::eStorageBuffer;
        write_ssbo.setBufferInfo(ssbo_info);

        device().updateDescriptorSets({write_textures, write_ssbo}, {});
    }
}

fi::Renderable fi::RenderMgr::get_renderable(DataIdx data_idx, size_t renderable_idx)
{
    Renderable rr{};
    rr.draw_call_ = &draw_calls_[data_idx][renderable_idx];
    rr.mat_ = &materials_[data_idx][renderable_idx];
    return rr;
}
