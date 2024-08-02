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

        vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
        vk::ImageCreateInfo img_info{
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Srgb,
            vk::Extent3D(img.width, img.height, 1),
            static_cast<uint32_t>(mip_mapping ? std::floor(std::log2(std::max(img.width, img.height))) + 1 : 1),
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

    for (const auto& mat_in : model_.materials)
    {
        Material& material = materials_.emplace_back();

        glms::assign_value(material.color_factor_, mat_in.pbrMetallicRoughness.baseColorFactor);
        material.metalic_ = mat_in.pbrMetallicRoughness.metallicFactor;
        material.roughtness_ = mat_in.pbrMetallicRoughness.roughnessFactor;
        material.color_texture_idx_ = mat_in.pbrMetallicRoughness.baseColorTexture.index;
        material.metalic_roughtness_ = mat_in.pbrMetallicRoughness.metallicRoughnessTexture.index;

        glms::assign_value(material.emissive_factor_, mat_in.emissiveFactor);
        material.emissive_factor_[3] = 1.0f;
        material.emissive_map_idx_ = mat_in.emissiveTexture.index;

        material.occlusion_map_idx_ = mat_in.occlusionTexture.index;

        material.alpha_cutoff_ = mat_in.alphaCutoff;

        material.normal_map_idx_ = mat_in.normalTexture.index;

        // extensions
        if (auto ext = mat_in.extensions.find("KHR_materials_emissive_strength"); ext != mat_in.extensions.end())
        {
            if (const auto& val = ext->second.Get("emissiveStrength"); val.Type() != gltf::NULL_TYPE)
            {
                material.emissive_factor_[3] = val.GetNumberAsDouble();
            }
        }

        if (auto ext = mat_in.extensions.find("KHR_materials_specular"); ext != mat_in.extensions.end())
        {
            if (const auto& val = ext->second.Get("specularFactor"); val.Type() != gltf::NULL_TYPE)
            {
                material.spec_factor_[3] = val.GetNumberAsDouble();
            }
            if (const auto& val = ext->second.Get("specularTexture"); val.Type() != gltf::NULL_TYPE)
            {
                material.spec_map_idx_ = val.GetNumberAsInt();
            }
            if (const auto& val = ext->second.Get("specularColorFactor"); val.Type() != gltf::NULL_TYPE)
            {
                for (size_t i = 0; i < 3; i++)
                {
                    material.spec_factor_[i] = val.Get(i).GetNumberAsDouble();
                }
            }
            if (const auto& val = ext->second.Get("specularColorTexture"); val.Type() != gltf::NULL_TYPE)
            {
                material.spec_color_map_idx_ = val.GetNumberAsInt();
            }
        }
    }
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
