#include "graphics/res_mesh.hpp"

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

fi::ResMeshDetails::ResMeshDetails(ResSceneDetails& target)
{
    std::vector<PrimVtx> vtxs;
    std::vector<uint32_t> idxs;
    std::vector<vk::DrawIndexedIndirectCommand> draw_calls;
    size_t old_vtx_count = 0;
    size_t old_idx_count = 0;

    first_gltf_mesh_.reserve(target.models().size());
    gltf_mesh_count_.reserve(target.models().size());
    for (auto& model_p : target.models())
    {
        const gltf::Model& model = *model_p;
        first_gltf_mesh_.push_back(meshes_.size());
        gltf_mesh_count_.push_back(model.meshes.size());

        uint32_t first_sampler = samplers_.size();
        samplers_.reserve(samplers_.size() + model.samplers.size());
        for (const auto& sampler : model.samplers)
        {
            vk::SamplerCreateInfo sampler_info{};
            sampler_info.addressModeU = decode_wrap_mode(sampler.wrapS);
            sampler_info.addressModeV = decode_wrap_mode(sampler.wrapT);
            sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
            decode_mipmap_mode(sampler.magFilter, &sampler_info.mipmapMode);
            sampler_info.maxLod =
                decode_mipmap_mode(sampler.magFilter, &sampler_info.mipmapMode) ? VK_LOD_CLAMP_NONE : 0;
            sampler_info.magFilter = decode_filter_mode(sampler.magFilter);
            sampler_info.minFilter = decode_filter_mode(sampler.minFilter);
            samplers_.push_back(device().createSampler(sampler_info));
        }

        uint32_t first_tex = tex_imgs_.size();
        tex_imgs_.reserve(tex_imgs_.size() + model.textures.size());
        tex_views_.reserve(tex_imgs_.size() + model.textures.size());
        tex_allocs_.reserve(tex_imgs_.size() + model.textures.size());
        tex_infos_.reserve(tex_imgs_.size() + model.textures.size());
        for (const auto& tex : model.textures)
        {
            const auto& img = model.images[tex.source];
            const auto& pixels = model.images[tex.source].image;
            bool mip_mapping = decode_mipmap_mode(model.samplers[tex.sampler].magFilter, nullptr);
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
                tex_imgs_.push_back(allocated.first);
                tex_allocs_.push_back(allocated.second);

                vk::ImageViewCreateInfo view_info{
                    {},
                    tex_imgs_.back(),
                    vk::ImageViewType::e2D,
                    img_info.format,
                    {},
                    {vk::ImageAspectFlagBits::eColor, 0, img_info.mipLevels, 0, img_info.arrayLayers}};
                tex_views_.push_back(device().createImageView(view_info));
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
            barrier.image = tex_imgs_.back();
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
                cmd.blitImage(tex_imgs_.back(), vk::ImageLayout::eTransferSrcOptimal, //
                              tex_imgs_.back(), vk::ImageLayout::eTransferDstOptimal, //
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

            vk::DescriptorImageInfo& tex_info = tex_infos_.emplace_back();
            tex_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            tex_info.imageView = tex_views_.back();
            tex_info.sampler = samplers_[first_sampler + tex.sampler];
        }

        uint32_t first_material = materials_.size();
        materials_.reserve(materials_.size() + model.materials.size());
        for (const auto& mat_in : model.materials)
        {
            PrimMaterial& material = materials_.emplace_back();

            glms::assign_value(material.color_factor_, mat_in.pbrMetallicRoughness.baseColorFactor);
            material.metalic_factor_ = mat_in.pbrMetallicRoughness.metallicFactor;
            material.roughtness_factor_ = mat_in.pbrMetallicRoughness.roughnessFactor;
            material.color_ = mat_in.pbrMetallicRoughness.baseColorTexture.index;
            material.metalic_roughness_ = mat_in.pbrMetallicRoughness.metallicRoughnessTexture.index;

            glms::assign_value(material.emissive_factor_, mat_in.emissiveFactor, 3);
            material.emissive_ = mat_in.emissiveTexture.index;
            material.occlusion_ = mat_in.occlusionTexture.index;
            material.normal_ = mat_in.normalTexture.index;

            if (mat_in.alphaMode == "OPAQUE")
            {
                material.alpha_cutoff_ = 0;
            }
            else if (mat_in.alphaMode == "MASK")
            {
                material.alpha_cutoff_ = mat_in.alphaCutoff;
            }
            else
            {
                material.alpha_cutoff_ = -1;
            }

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
                    material.specular_factor_[3] = val.GetNumberAsDouble();
                }
                if (check_ext("specularTexture"))
                {
                    material.specular_ = val.GetNumberAsInt();
                }
                if (check_ext("specularColorFactor"))
                {
                    for (size_t i = 0; i < 3; i++)
                    {
                        material.specular_factor_[i] = val.Get(i).GetNumberAsDouble();
                    }
                }
                if (check_ext("specularColorTexture"))
                {
                    material.specular_color_ = val.GetNumberAsInt();
                }
            }

            material.color_ += material.color_ == -1 ? 0 : first_tex;
            material.metalic_roughness_ += (material.metalic_roughness_ == -1 ? 0 : first_tex);
            material.normal_ += (material.normal_ == -1 ? 0 : first_tex);
            material.emissive_ += (material.emissive_ == -1 ? 0 : first_tex);
            material.occlusion_ += (material.occlusion_ == -1 ? 0 : first_tex);
            material.anistropy_ += (material.anistropy_ == -1 ? 0 : first_tex);
            material.specular_ += (material.specular_ == -1 ? 0 : first_tex);
            material.specular_color_ += (material.specular_color_ == -1 ? 0 : first_tex);
            material.sheen_color_ += (material.sheen_color_ == -1 ? 0 : first_tex);
            material.sheen_roughtness_ += (material.sheen_roughtness_ == -1 ? 0 : first_tex);
        }

        uint32_t first_mesh = meshes_.size();
        uint32_t first_primitive = prim_details_.size();
        meshes_.reserve(meshes_.size() + model.meshes.size());
        mesh_details_.reserve(mesh_details_.size() + model.meshes.size());
        for (const auto& mesh : model.meshes)
        {
            ResMesh& res_mesh = meshes_.emplace_back();
            res_mesh.name_ = mesh.name;
            res_mesh.first_prim_ = prim_details_.size();
            res_mesh.prim_count_ = mesh.primitives.size();
            res_mesh.details_ = &mesh_details_.emplace_back();
            prim_details_.reserve(prim_details_.size() + mesh.primitives.size());

            for (const auto& prim : mesh.primitives)
            {
                PrimDetails& prim_detail = prim_details_.emplace_back();
                prim_detail.material_ = first_material + prim.material;
                prim_detail.mesh_ = meshes_.size() - 1;

                std::future<void> idx_async = std::async(
                    [&]()
                    {
                        const gltf::Accessor& idx_acc = model.accessors[prim.indices];
                        idxs.reserve(old_idx_count + idx_acc.count);
                        iterate_acc(
                            [&](size_t iter_idx, const unsigned char* data, size_t size)
                            {
                                if (size == 2)
                                {
                                    idxs.emplace_back(*castf(uint16_t*, data));
                                }
                                else
                                {
                                    idxs.emplace_back(*castf(uint32_t*, data));
                                }
                            },
                            idx_acc, model);
                        vk::DrawIndexedIndirectCommand& draw_call = draw_calls.emplace_back();
                        draw_call.firstIndex = old_idx_count;
                        draw_call.indexCount = idx_acc.count;
                        draw_call.vertexOffset = old_vtx_count;
                        draw_call.firstInstance = 0;
                        draw_call.instanceCount = 1;
                        old_idx_count = idxs.size();
                    });
                const gltf::Accessor& pos_acc = model.accessors[prim.attributes.at("POSITION")];
                vtxs.resize(old_vtx_count + pos_acc.count);
                std::future<void> position_async = std::async(
                    [&]()
                    {
                        iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                                    { glms::assign_value(vtxs[old_vtx_count + idx].position_, (float*)data, 3); }, //
                                    pos_acc, model);
                    });
                std::future<void> normal_async = std::async(
                    [&]()
                    {
                        auto result = prim.attributes.find("NORMAL");
                        if (result == prim.attributes.end())
                        {
                            return;
                        }
                        const gltf::Accessor& acc = model.accessors[result->second];
                        iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                                    { glms::assign_value(vtxs[old_vtx_count + idx].normal_, (float*)data, 3); }, //
                                    acc, model);
                    });
                std::future<void> tangent_async = std::async(
                    [&]()
                    {
                        auto result = prim.attributes.find("TANGENT");
                        if (result == prim.attributes.end())
                        {
                            return;
                        }
                        const gltf::Accessor& acc = model.accessors[result->second];
                        iterate_acc([&](size_t idx, const unsigned char* data, size_t size)
                                    { glms::assign_value(vtxs[old_vtx_count + idx].tangent_, (float*)data, 4); }, //
                                    acc, model);
                    });
                std::future<void> tex_coord_async = std::async(
                    [&]()
                    {
                        auto result = prim.attributes.find("TEXCOORD_0");
                        if (result == prim.attributes.end())
                        {
                            return;
                        }
                        const gltf::Accessor& acc = model.accessors[result->second];
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
                            acc, model);
                    });
                std::future<void> color_async = std::async(
                    [&]()
                    {
                        auto result = prim.attributes.find("COLOR_0");
                        if (result == prim.attributes.end())
                        {
                            return;
                        }
                        const gltf::Accessor& acc = model.accessors[result->second];
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
                            acc, model);
                    });
                std::future<void> joint_async = std::async(
                    [&]()
                    {
                        auto result = prim.attributes.find("JOINTS_0");
                        if (result == prim.attributes.end())
                        {
                            return;
                        }
                        const gltf::Accessor& acc = model.accessors[result->second];
                        iterate_acc(
                            [&](size_t idx, const unsigned char* data, size_t size)
                            {
                                glm::uvec4& joint = vtxs[old_vtx_count + idx].joint_;
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
                            acc, model);
                    });
                std::future<void> weight_async = std::async(
                    [&]()
                    {
                        auto result = prim.attributes.find("WEIGHTS_0");
                        if (result == prim.attributes.end())
                        {
                            return;
                        }
                        const gltf::Accessor& acc = model.accessors[result->second];
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
                                            weight[i / 2] = get_normalized(castf(uint16_t*, data));
                                        }
                                        break;
                                    case 16:
                                        glms::assign_value(weight, castf(float*, data), 4);
                                        break;
                                }
                            }, //
                            acc, model);
                    });

                idx_async.wait();
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
    }

    // calculateing padding for buffer
    vk::DeviceSize ssbo_front_padding = draw_calls.size() % 16;
    vk::DeviceSize mat_back_padding = 0;
    vk::DeviceSize prim_back_padding = 0;
    vk::DeviceSize mesh_back_padding = 0;
}

fi::ResMeshDetails::~ResMeshDetails()
{
}
