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
    std::vector<PrimVtx> vtx;
    std::vector<uint32_t> idx;
    std::vector<vk::DrawIndexedIndirectCommand> draw_calls;
    size_t old_vtx_count = 0;
    size_t old_idx_count = 0;

    first_gltf_mesh_.reserve(target.models().size());
    gltf_mesh_count_.reserve(first_gltf_mesh_.size());
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
        tex_views_.reserve(tex_imgs_.size());
        tex_allocs_.reserve(tex_imgs_.size());
        tex_infos_.reserve(tex_imgs_.size());
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

                material.color_ += first_tex;
                material.metalic_roughness_ += first_tex;
                material.normal_ += first_tex;
                material.emissive_ += first_tex;
                material.occlusion_ += first_tex;
                material.anistropy_ += first_tex;
                material.specular_ += first_tex;
                material.specular_color_ += first_tex;
                material.sheen_color_ += first_tex;
                material.sheen_roughtness_ += first_tex;
            }
        }
    }
}
