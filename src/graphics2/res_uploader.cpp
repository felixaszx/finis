#include "graphics2/res_uploader.hpp"

vk::SamplerAddressMode decode_wrap_mode(int mode)
{
    switch (mode)
    {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return vk::SamplerAddressMode::eClampToEdge;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return vk::SamplerAddressMode::eRepeat;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return vk::SamplerAddressMode::eMirroredRepeat;
    }
    return vk::SamplerAddressMode::eClampToEdge;
};

vk::Filter decode_filter_mode(int mode)
{
    switch (mode)
    {
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            return vk::Filter::eNearest;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            return vk::Filter::eLinear;
    }
    return vk::Filter::eLinear;
};

template <typename T>
float get_normalized(T integer)
{
    return integer / (float)std::numeric_limits<T>::max();
}

template <typename T>
float get_normalized(T* integer)
{
    return get_normalized(*integer);
}

bool decode_mipmap_mode(int mode, vk::SamplerMipmapMode* out_mode)
{
    switch (mode)
    {
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            if (out_mode)
            {
                *out_mode = vk::SamplerMipmapMode::eNearest;
            }
            return true;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            if (out_mode)
            {
                *out_mode = vk::SamplerMipmapMode::eLinear;
            }
            return true;
    }
    return false;
};

void fi::ResDetails::load_mip_maps()
{
}

fi::ResDetails::ResDetails(const std::filesystem::path& path)
{
    gltf_loader().SetImagesAsIs(false);
    gltf_loader().SetPreserveImageChannels(false);
    std::string err = "";
    std::string warnning = "";
    bool loaded = false;
    if (path.extension() == ".glb")
    {
        loaded = gltf_loader().LoadBinaryFromFile(&model_, &err, &warnning, path.generic_string());
    }
    else if (path.extension() == ".gltf")
    {
        loaded = gltf_loader().LoadASCIIFromFile(&model_, &err, &warnning, path.generic_string());
    }

    if (!loaded)
    {
        std::cout << std::format("{}\n{}\n", err, warnning);
        throw std::runtime_error(std::format("Fail to load {}.\n", path.generic_string()));
        return;
    }

    for (const auto& sampler : model_.samplers)
    {
        vk::SamplerCreateInfo sampler_info{};
        sampler_info.addressModeU = decode_wrap_mode(sampler.wrapS);
        sampler_info.addressModeV = decode_wrap_mode(sampler.wrapT);
        sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
        decode_mipmap_mode(sampler.magFilter, &sampler_info.mipmapMode);
        sampler_info.maxLod = decode_mipmap_mode(sampler.magFilter, &sampler_info.mipmapMode) ? VK_LOD_CLAMP_NONE : 0;
        sampler_info.magFilter = decode_filter_mode(sampler.magFilter);
        sampler_info.minFilter = decode_filter_mode(sampler.minFilter);

        samplers_.push_back(device().createSampler(sampler_info));
    }

    for (const auto& tex : model_.textures)
    {
        const auto& img = model_.images[tex.source];
        const auto& pixels = model_.images[tex.source].image;
        bool mip_mapping = decode_mipmap_mode(model_.samplers[tex.sampler].magFilter, nullptr);
        uint32_t levels = mip_mapping ? std::floor(std::log2(std::max(img.width, img.height))) + 1 : 1;

        {
            vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
            vk::ImageCreateInfo img_info{{},
                                         vk::ImageType::e2D,
                                         vk::Format::eR8G8B8A8Srgb,
                                         vk::Extent3D(img.width, img.height, 1),
                                         casts(uint32_t, levels),
                                         1,
                                         vk::SampleCountFlagBits::e1,
                                         vk::ImageTiling::eOptimal,
                                         vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst |
                                             vk::ImageUsageFlagBits::eTransferSrc};
            auto allocated = allocator().createImage(img_info, alloc_info);
            textures_.push_back(allocated.first);
            texture_allocs_.push_back(allocated.second);

            vk::ImageViewCreateInfo view_info{
                {},
                textures_.back(),
                vk::ImageViewType::e2D,
                img_info.format,
                {},
                {vk::ImageAspectFlagBits::eColor, 0, img_info.mipLevels, 0, img_info.arrayLayers}};
            texture_views_.push_back(device().createImageView(view_info));
        }

        vk::BufferCreateInfo buffer_info{};
        buffer_info.size = pixels.size();
        buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
        vma::AllocationCreateInfo alloc_info{};
        alloc_info.usage = vma::MemoryUsage::eAutoPreferHost;
        alloc_info.preferredFlags = vk::MemoryPropertyFlagBits::eHostCoherent;
        alloc_info.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
        auto staging = allocator().createBuffer(buffer_info, alloc_info);
        void* mapping = allocator().mapMemory(staging.second);
        memcpy(mapping, pixels.data(), buffer_info.size);
        allocator().unmapMemory(staging.second);

        vk::ImageMemoryBarrier barrier{};
        barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = textures_.back();
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        vk::BufferImageCopy region{};
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = vk::Extent3D(img.width, img.height, 1);

        vk::CommandBuffer cmd = one_time_submit_cmd();
        begin_cmd(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);
        cmd.copyBufferToImage(staging.first, barrier.image, vk::ImageLayout::eTransferDstOptimal, region);

        int32_t mip_w = img.width;
        int32_t mip_h = img.height;
        for (int i = 1; i < levels; i++)
        {
            barrier.subresourceRange.levelCount = 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.subresourceRange.baseMipLevel = i - 1;
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                                {}, {}, {}, barrier);

            vk::ImageBlit blit{};
            blit.srcOffsets[1] = vk::Offset3D(mip_w, mip_h, 1);
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[1] = vk::Offset3D(mip_w > 1 ? mip_w / 2 : 1, //
                                              mip_h > 1 ? mip_h / 2 : 1, //
                                              1);
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            cmd.blitImage(textures_.back(), vk::ImageLayout::eTransferSrcOptimal, //
                          textures_.back(), vk::ImageLayout::eTransferDstOptimal, //
                          blit, vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.subresourceRange.baseMipLevel = i - 1;
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                                {}, {}, {}, barrier);

            mip_w > 1 ? mip_w /= 2 : mip_w;
            mip_h > 1 ? mip_h /= 2 : mip_h;
        }
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.subresourceRange.baseMipLevel = levels - 1;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);
        cmd.end();
        submit_one_time_cmd(cmd);
        allocator().destroyBuffer(staging.first, staging.second);
    }

    for (const auto& mat_in : model_.materials)
    {
        Material& material = materials_.emplace_back();

        glms::assign_value(material.color_factor_, mat_in.pbrMetallicRoughness.baseColorFactor);
        material.metalic_ = mat_in.pbrMetallicRoughness.metallicFactor;
        material.roughtness_ = mat_in.pbrMetallicRoughness.roughnessFactor;
        material.color_texture_idx_ = mat_in.pbrMetallicRoughness.baseColorTexture.index;
        material.metalic_roughtness_ = mat_in.pbrMetallicRoughness.metallicRoughnessTexture.index;

        glms::assign_value(material.emissive_factor_, mat_in.emissiveFactor, 3);
        material.emissive_map_idx_ = mat_in.emissiveTexture.index;

        material.occlusion_map_idx_ = mat_in.occlusionTexture.index;

        material.alpha_cutoff_ = mat_in.alphaCutoff;

        material.normal_map_idx_ = mat_in.normalTexture.index;

// extensions
#define check_ext(name)                      \
    const auto& val = ext->second.Get(name); \
    val.Type() != gltf::NULL_TYPE
        if (auto ext = mat_in.extensions.find("KHR_materials_emissive_strength"); ext != mat_in.extensions.end())
        {
            if (check_ext("emissiveStrength"))
            {
                material.emissive_factor_[3] = val.GetNumberAsDouble();
            }
        }

        if (auto ext = mat_in.extensions.find("KHR_materials_specular"); ext != mat_in.extensions.end())
        {
            if (check_ext("specularFactor"))
            {
                material.spec_factor_[3] = val.GetNumberAsDouble();
            }
            if (check_ext("specularTexture"))
            {
                material.spec_map_idx_ = val.GetNumberAsInt();
            }
            if (check_ext("specularColorFactor"))
            {
                for (size_t i = 0; i < 3; i++)
                {
                    material.spec_factor_[i] = val.Get(i).GetNumberAsDouble();
                }
            }
            if (check_ext("specularColorTexture"))
            {
                material.spec_color_map_idx_ = val.GetNumberAsInt();
            }
        }
    }

    std::vector<Vtx> vtxs;
    std::vector<uint32_t> idxs;
    std::vector<vk::DrawIndexedIndirectCommand> draw_calls;
    std::vector<glm::mat4> instance_mats;
    size_t old_vtx_count = 0;
    size_t old_idx_count = 0;

    auto iterate_acc = [](const std::function<void(size_t idx, const unsigned char* data, size_t size)>& cb, //
                          const gltf::Accessor& acc,                                                         //
                          const gltf::Model& model)
    {
        const gltf::BufferView& view = model.bufferViews[acc.bufferView];
        const gltf::Buffer& buffer = model.buffers[view.buffer];
        size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                      * gltf::GetNumComponentsInType(acc.type);
        size_t stride = view.byteStride ? view.byteStride : size;
        size_t offset = view.byteOffset + acc.byteOffset;

        if (acc.sparse.isSparse)
        {
            const gltf::BufferView& sparse_view = model.bufferViews[acc.sparse.indices.bufferView];
            const gltf::Buffer& sparse_buffer = model.buffers[sparse_view.buffer];
            const gltf::BufferView& deviate_view = model.bufferViews[acc.sparse.values.bufferView];
            const gltf::Buffer& deviate_buffer = model.buffers[deviate_view.buffer];
            size_t sparse_size = gltf::GetComponentSizeInBytes(acc.sparse.indices.componentType);
            size_t sparse_offset = sparse_view.byteOffset + acc.sparse.indices.byteOffset;
            size_t deviate_offset = deviate_view.byteOffset + acc.sparse.values.byteOffset;

            std::vector<size_t> deviate_idxs;
            deviate_idxs.reserve(acc.sparse.count);
            for (size_t i = 0; i < deviate_idxs.size(); i++)
            {
                const unsigned char* sparse_ptr = (sparse_buffer.data.data() + sparse_offset + sparse_size * i);
                switch (sparse_size)
                {
                    case 1:
                        deviate_idxs.push_back(*sparse_ptr);
                        break;
                    case 2:
                        deviate_idxs.push_back(*castf(uint16_t*, sparse_ptr));
                        break;
                    case 4:
                        deviate_idxs.push_back(*castf(uint32_t*, sparse_ptr));
                        break;
                }
            }

            size_t deviate_idx = 0;
            for (size_t b = 0; b < acc.count; b++)
            {
                if (deviate_idxs[deviate_idx] == b)
                {
                    cb(b, deviate_buffer.data.data() + deviate_offset + deviate_idx * size, size);
                    deviate_idx++;
                }
                else
                {
                    cb(b, buffer.data.data() + offset + b * stride, size);
                }
            }
        }
        else
        {
            for (size_t b = 0; b < acc.count; b++)
            {
                cb(b, buffer.data.data() + offset + b * stride, size);
            }
        }
    };

    for (const auto& mesh : model_.meshes)
    {
        size_t prim_idx = 0;
        for (const auto& prim : mesh.primitives)
        {
            material_idxs_.push_back(prim.material);
            prim_names_.push_back(mesh.name + std::format("_prim_{}", prim_idx));
            prim_idx++;
            {
                const gltf::Accessor& idx_acc = model_.accessors[prim.indices];
                const gltf::BufferView& idx_view = model_.bufferViews[idx_acc.bufferView];
                const gltf::Buffer& idx_buffer = model_.buffers[idx_view.buffer];
                idxs.reserve(old_idx_count + idx_acc.count);

                size_t idx_size = gltf::GetComponentSizeInBytes(idx_acc.componentType) //
                                  * gltf::GetNumComponentsInType(idx_acc.type);
                size_t stride = idx_view.byteStride ? idx_view.byteStride : idx_size;
                size_t offset = idx_view.byteOffset + idx_acc.byteOffset;
                for (size_t b = offset; b < offset + idx_acc.count * idx_size; b += stride)
                {
                    if (idx_size == 2)
                    {
                        idxs.emplace_back(*castf(uint16_t*, idx_buffer.data.data() + b));
                    }
                    else
                    {
                        idxs.emplace_back(*castf(uint32_t*, idx_buffer.data.data() + b));
                    }
                }
                vk::DrawIndexedIndirectCommand& draw_call = draw_calls.emplace_back();
                draw_call.firstIndex = old_idx_count;
                draw_call.indexCount = idx_acc.count;
                draw_call.vertexOffset = old_vtx_count;
                draw_call.firstInstance = 0;
                draw_call.instanceCount = 1;
                old_idx_count = idxs.size();
            }

            const gltf::Accessor& pos_acc = model_.accessors[prim.attributes.at("POSITION")];
            vtxs.resize(old_vtx_count + pos_acc.count);
            std::future<void> position_async = std::async(
                [&]()
                {
                    iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                                { glms::assign_value(vtxs[old_vtx_count + idx].position_, (float*)data, 3); }, //
                                pos_acc, model_);
                });
            std::future<void> normal_async = std::async(
                [&]()
                {
                    auto result = prim.attributes.find("NORMAL");
                    if (result == prim.attributes.end())
                    {
                        return;
                    }
                    const gltf::Accessor& acc = model_.accessors[result->second];
                    iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                                { glms::assign_value(vtxs[old_vtx_count + idx].normal_, (float*)data, 3); }, //
                                acc, model_);
                });
            std::future<void> tangent_async = std::async(
                [&]()
                {
                    auto result = prim.attributes.find("TANGENT");
                    if (result == prim.attributes.end())
                    {
                        return;
                    }
                    const gltf::Accessor& acc = model_.accessors[result->second];
                    iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                                { glms::assign_value(vtxs[old_vtx_count + idx].tangent_, (float*)data, 4); }, //
                                acc, model_);
                });
            std::future<void> tex_coord_async = std::async(
                [&]()
                {
                    auto result = prim.attributes.find("TEXCOORD_0");
                    if (result == prim.attributes.end())
                    {
                        return;
                    }
                    const gltf::Accessor& acc = model_.accessors[result->second];
                    iterate_acc(
                        [&](size_t idx, const unsigned char* data, size_t size)
                        {
                            glm::vec2& tex_coord = vtxs[old_vtx_count + idx].tex_coord_;
                            switch (size)
                            {
                                case 2:
                                    tex_coord[0] = get_normalized(castf(uint8_t*, data));
                                    tex_coord[1] = get_normalized(castf(uint8_t*, data + sizeof(uint8_t)));
                                    break;
                                case 4:
                                    tex_coord[0] = get_normalized(castf(uint16_t*, data));
                                    tex_coord[1] = get_normalized(castf(uint16_t*, data + sizeof(uint16_t)));
                                    break;
                                case 8:
                                    glms::assign_value(tex_coord, castf(float*, data), 2);
                                    break;
                            }
                        }, //
                        acc, model_);
                });
            std::future<void> color_async = std::async(
                [&]()
                {
                    auto result = prim.attributes.find("COLOR_0");
                    if (result == prim.attributes.end())
                    {
                        return;
                    }
                    const gltf::Accessor& acc = model_.accessors[result->second];
                    iterate_acc(
                        [&](size_t idx, const unsigned char* data, size_t size)
                        {
                            glm::vec4& color = vtxs[old_vtx_count + idx].color_;
                            color[3] = 1.0f;
                            switch (size)
                            {
                                case 3:
                                case 4:
                                    for (int i = 0; i < size; i += sizeof(uint8_t))
                                    {
                                        color[i] = get_normalized(castf(uint8_t*, data + i));
                                    }
                                    break;
                                case 6:
                                case 8:
                                    for (int i = 0; i < size; i += sizeof(uint16_t))
                                    {
                                        color[i / 2] = get_normalized(castf(uint16_t*, data + i));
                                    }
                                    break;
                                case 12:
                                case 16:
                                    glms::assign_value(color, castf(float*, data), size / 4);
                                    break;
                            }
                        }, //
                        acc, model_);
                });
            std::future<void> joint_async = std::async(
                [&]()
                {
                    auto result = prim.attributes.find("JOINTS_0");
                    if (result == prim.attributes.end())
                    {
                        return;
                    }
                    const gltf::Accessor& acc = model_.accessors[result->second];
                    iterate_acc(
                        [&](size_t idx, const unsigned char* data, size_t size)
                        {
                            glm::u16vec4& joint = vtxs[idx].joint_;
                            switch (size)
                            {
                                case 4:
                                    glms::assign_value(joint, castf(uint8_t*, data), 4);
                                    break;
                                case 8:
                                    glms::assign_value(joint, castf(uint16_t*, data), 4);
                                    break;
                            }
                        }, //
                        acc, model_);
                });
            std::future<void> weight_async = std::async(
                [&]()
                {
                    auto result = prim.attributes.find("WEIGHTS_0");
                    if (result == prim.attributes.end())
                    {
                        return;
                    }
                    const gltf::Accessor& acc = model_.accessors[result->second];
                    iterate_acc(
                        [&](size_t idx, const unsigned char* data, size_t size)
                        {
                            glm::vec4& weight = vtxs[old_vtx_count + idx].weight_;
                            switch (size)
                            {
                                case 4:
                                    for (int i = 0; i < size; i += sizeof(uint8_t))
                                    {
                                        weight[i] = get_normalized(castf(uint8_t*, data));
                                    }
                                    break;
                                case 8:
                                    for (int i = 0; i < size; i += sizeof(uint16_t))
                                    {
                                        weight[i / 2] = get_normalized(castf(uint8_t*, data));
                                    }
                                    break;
                                case 16:
                                    glms::assign_value(weight, castf(float*, data), 4);
                                    break;
                            }
                        }, //
                        acc, model_);
                });

            position_async.wait();
            normal_async.wait();
            tangent_async.wait();
            tex_coord_async.wait();
            color_async.wait();
            joint_async.wait();
            weight_async.wait();
            old_vtx_count = vtxs.size();
        }
    }

    while (sizeof_arr(material_idxs_) % 16)
    {
        material_idxs_.push_back(-1);
    }

    make_unique2(buffer_,
                 sizeof_arr(vtxs)                 //
                     + sizeof_arr(idxs)           //
                     + sizeof_arr(materials_)     //
                     + sizeof_arr(material_idxs_) //
                     + sizeof_arr(draw_calls),
                 DST);
    Buffer<BufferBase::EmptyExtraInfo, vertex, seq_write> staging(buffer_->size(), SRC);

    buffer_->idx_buffer_ = sizeof_arr(vtxs);
    buffer_->materials_ = buffer_->idx_buffer_ + sizeof_arr(idxs);
    buffer_->material_idxs_ = buffer_->materials_ + sizeof_arr(materials_);
    buffer_->draw_calls_ = buffer_->material_idxs_ + sizeof_arr(material_idxs_);

    memcpy(staging.map_memory(), vtxs.data(), sizeof_arr(vtxs));
    memcpy(staging.mapping() + buffer_->idx_buffer_, idxs.data(), sizeof_arr(idxs));
    memcpy(staging.mapping() + buffer_->materials_, materials_.data(), sizeof_arr(materials_));
    memcpy(staging.mapping() + buffer_->material_idxs_, material_idxs_.data(), sizeof_arr(material_idxs_));
    memcpy(staging.mapping() + buffer_->draw_calls_, draw_calls.data(), sizeof_arr(draw_calls));

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    cmd.copyBuffer(staging, *buffer_, {{0, 0, buffer_->size()}});
    cmd.end();
    submit_one_time_cmd(cmd);
}

fi::ResDetails::~ResDetails()
{
    for (size_t i = 0; i < textures_.size(); i++)
    {
        device().destroyImageView(texture_views_[i]);
        allocator().destroyImage(textures_[i], texture_allocs_[i]);
    }

    for (auto sampler : samplers_)
    {
        device().destroySampler(sampler);
    }
}
