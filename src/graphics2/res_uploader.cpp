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

        vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
        vk::ImageCreateInfo img_info{
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Srgb,
            vk::Extent3D(img.width, img.height, 1),
            casts(uint32_t, mip_mapping ? std::floor(std::log2(std::max(img.width, img.height))) + 1 : 1),
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

    std::vector<uint32_t> idxs;
    std::vector<vk::DrawIndexedIndirectCommand> draw_calls;
    size_t old_vtx_count = 0;
    size_t old_idx_count = 0;

    for (const auto& mesh : model_.meshes)
    {
        size_t prim_idx = 0;
        for (const auto& prim : mesh.primitives)
        {
            prim_names_.push_back(mesh.name + "_" + std::to_string(prim_idx));
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

            std::future<void> position_async = std::async(
                [&]()
                {
                    const gltf::Accessor& acc = model_.accessors[prim.attributes.at("POSITION")];
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    positions_.reserve(old_vtx_count + acc.count);

                    size_t pos_size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                      * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : pos_size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * pos_size; b += stride)
                    {
                        glms::assign_value(positions_.emplace_back(), castf(float*, buffer.data.data() + b), 3);
                    }
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
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    normals_.reserve(old_vtx_count + acc.count);

                    size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                  * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * size; b += stride)
                    {
                        glms::assign_value(normals_.emplace_back(), castf(float*, buffer.data.data() + b), 3);
                    }
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
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    tangents_.reserve(old_vtx_count + acc.count);

                    size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                  * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * size; b += stride)
                    {
                        glms::assign_value(tangents_.emplace_back(), castf(float*, buffer.data.data() + b), 4);
                    }
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
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    tex_coords_.reserve(old_vtx_count + acc.count);

                    size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                  * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * size; b += stride)
                    {
                        glm::vec2& tex_coord = tex_coords_.emplace_back();
                        switch (size)
                        {
                            case 2:
                                tex_coord[0] = get_normalized(castf(uint8_t*, buffer.data.data() + b));
                                tex_coord[1] =
                                    get_normalized(castf(uint8_t*, buffer.data.data() + b + sizeof(uint8_t)));
                                break;
                            case 4:
                                tex_coord[0] = get_normalized(castf(uint16_t*, buffer.data.data() + b));
                                tex_coord[1] =
                                    get_normalized(castf(uint16_t*, buffer.data.data() + b + sizeof(uint16_t)));
                                break;
                            case 8:
                                glms::assign_value(tex_coord, castf(float*, buffer.data.data() + b), 2);
                                break;
                        }
                    }
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
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    colors_.reserve(old_vtx_count + acc.count);

                    size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                  * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * size; b += stride)
                    {
                        glm::vec4& color = colors_.emplace_back();
                        color[3] = 1.0f;
                        switch (size)
                        {
                            case 3:
                            case 4:
                                for (int i = 0; i < size; i += sizeof(uint8_t))
                                {
                                    color[i] = get_normalized(castf(uint8_t*, buffer.data.data() + b + i));
                                }
                                break;
                            case 6:
                            case 8:
                                for (int i = 0; i < size; i += sizeof(uint16_t))
                                {
                                    color[i / 2] = get_normalized(castf(uint16_t*, buffer.data.data() + b + i));
                                }
                                break;
                            case 12:
                            case 16:
                                glms::assign_value(color, castf(float*, buffer.data.data() + b), size / 4);
                                break;
                        }
                    }
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
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    joints_.reserve(old_vtx_count + acc.count);

                    size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                  * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * size; b += stride)
                    {
                        switch (size)
                        {
                            case 4:
                                glms::assign_value(joints_.emplace_back(), castf(uint8_t*, buffer.data.data() + b), 4);
                                break;
                            case 8:
                                glms::assign_value(joints_.emplace_back(), castf(uint16_t*, buffer.data.data() + b), 4);
                                break;
                        }
                    }
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
                    const gltf::BufferView& view = model_.bufferViews[acc.bufferView];
                    const gltf::Buffer& buffer = model_.buffers[view.buffer];
                    weights_.reserve(old_vtx_count + acc.count);

                    size_t size = gltf::GetComponentSizeInBytes(acc.componentType) //
                                  * gltf::GetNumComponentsInType(acc.type);
                    size_t stride = view.byteStride ? view.byteStride : size;
                    size_t offset = view.byteOffset + acc.byteOffset;
                    for (size_t b = offset; b < offset + acc.count * size; b += stride)
                    {
                        glm::vec4& weight = weights_.emplace_back();
                        switch (size)
                        {
                            case 4:
                                for (int i = 0; i < size; i += sizeof(uint8_t))
                                {
                                    weight[i] = get_normalized(castf(uint8_t*, buffer.data.data() + b + i));
                                }
                                break;
                            case 8:
                                for (int i = 0; i < size; i += sizeof(uint16_t))
                                {
                                    weight[i / 2] = get_normalized(castf(uint8_t*, buffer.data.data() + b + i));
                                }
                                break;
                            case 16:
                                glms::assign_value(weight, castf(float*, buffer.data.data() + b), 4);
                                break;
                        }
                    }
                });

            position_async.wait();
            normal_async.wait();
            tangent_async.wait();
            tex_coord_async.wait();
            color_async.wait();
            joint_async.wait();
            weight_async.wait();
            old_vtx_count = positions_.size();
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
