#define STB_IMAGE_IMPLEMENTATION
#include "resources/gltf_file.hpp"

template <typename T>
void check_attribute(const std::string& name,
                     const fi::res::fgltf::Primitive& prim,
                     const fi::res::fgltf::Asset& asset,
                     std::vector<T>& target)
{
    const fi::res::fgltf::Attribute* attrib = prim.findAttribute(name);
    if (attrib != prim.attributes.end())
    {
        const fi::res::fgltf::Accessor& acc = asset.accessors[attrib->accessorIndex];
        target.resize(acc.count);
        fi::res::fgltf::iterateAccessorWithIndex<T>(asset, acc, //
                                                    [&](const T& elm, size_t index) { target[index] = elm; });
    }
};

static auto check_mipmapped = [](fastgltf::Filter filter)
{
    switch (filter)
    {
        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return 1000.0f;
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::Linear:
            return 0.0f;
    }
    return 0.0f;
};

fi::res::gltf_file::gltf_file(const std::filesystem::path& path,
                              std::vector<std::future<void>>* futs,
                              thp::task_thread_pool* th_pool)
{
    fgltf::Parser parser;
    auto data = fgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fgltf::Error::None)
    {
        throw std::runtime_error(std::format("Fail to load gltf {}", path.string()));
    }

    auto asset_in = parser.loadGltf(data.get(), path.parent_path(),
                                    fgltf::Options::LoadExternalBuffers |    //
                                        fgltf::Options::LoadExternalImages | //
                                        fgltf::Options::GenerateMeshIndices);
    if (auto error = asset_in.error(); error != fgltf::Error::None)
    {
        throw std::runtime_error(std::format("Fail to parse gltf {}", path.string()));
    }

    if (auto error = fgltf::validate(asset_in.get()); error != fgltf::Error::None)
    {
        throw std::runtime_error(std::format("gltf {} is not valid", path.string()));
    }
    util::make_unique2(asset_, std::move(asset_in.get()));
    name_ = path.filename().generic_string();

    samplers_.reserve(asset_->samplers.size());
    for (const fgltf::Sampler& sampler : asset_->samplers)
    {
        vk::SamplerCreateInfo& info = samplers_.emplace_back();

        auto extract_filter = [](fgltf::Filter filter)
        {
            switch (filter)
            {
                case fastgltf::Filter::NearestMipMapNearest:
                case fastgltf::Filter::LinearMipMapNearest:
                case fastgltf::Filter::Nearest:
                    return vk::Filter::eNearest;
                case fastgltf::Filter::NearestMipMapLinear:
                case fastgltf::Filter::LinearMipMapLinear:
                case fastgltf::Filter::Linear:
                    return vk::Filter::eLinear;
            }
            return vk::Filter::eLinear;
        };

        auto extract_mipmap_mode = [](fgltf::Filter filter)
        {
            switch (filter)
            {
                case fastgltf::Filter::NearestMipMapLinear:
                case fastgltf::Filter::NearestMipMapNearest:
                    return vk::SamplerMipmapMode::eNearest;
                case fastgltf::Filter::Nearest:
                case fastgltf::Filter::Linear:
                case fastgltf::Filter::LinearMipMapLinear:
                case fastgltf::Filter::LinearMipMapNearest:
                    return vk::SamplerMipmapMode::eLinear;
            }
            return vk::SamplerMipmapMode::eLinear;
        };

        auto extract_wrap = [](fgltf::Wrap wrap)
        {
            switch (wrap)
            {
                case fastgltf::Wrap::ClampToEdge:
                    return vk::SamplerAddressMode::eClampToEdge;
                case fastgltf::Wrap::MirroredRepeat:
                    return vk::SamplerAddressMode::eMirroredRepeat;
                case fastgltf::Wrap::Repeat:
                    return vk::SamplerAddressMode::eRepeat;
            }
            return vk::SamplerAddressMode::eRepeat;
        };

        const fgltf::Filter filter = fgltf::Filter::Linear;
        info.addressModeU = extract_wrap(sampler.wrapS);
        info.addressModeV = extract_wrap(sampler.wrapT);
        info.addressModeW = vk::SamplerAddressMode::eRepeat;
        info.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(filter));
        info.minFilter = extract_filter(sampler.minFilter.value_or(filter));
        info.magFilter = extract_filter(sampler.magFilter.value_or(filter));
        info.borderColor = vk::BorderColor::eIntOpaqueBlack;
        info.anisotropyEnable = true;
        info.maxAnisotropy = 4.0f;
        info.minLod = 0.0f;
        info.maxLod = check_mipmapped(sampler.minFilter.value_or(filter));
    }

    auto load_tex_func = [&]()
    {
        textures_.reserve(asset_->textures.size());
        for (const auto& tex : asset_->textures)
        {
            const fgltf::Image& img = asset_->images[tex.imageIndex.value()];
            const fgltf::BufferView& view = asset_->bufferViews[std::get<1>(img.data).bufferViewIndex];
            const fgltf::Buffer& buffer = asset_->buffers[view.bufferIndex];

            gltf_tex& g_tex = textures_.emplace_back();
            g_tex.name_ = std::format("{}__image({})", tex.name, img.name);
            stbi_uc* pixels = stbi_load_from_memory(
                (const stbi_uc*)(std::get<3>(buffer.data).bytes.data() + view.byteOffset),
                view.byteLength, //
                &g_tex.x_, &g_tex.y_, &g_tex.comp_, STBI_rgb_alpha);
            g_tex.comp_ = 4;
            g_tex.sampler_idx_ = *tex.samplerIndex;
            g_tex.mipmapped_ = check_mipmapped(asset_
                                                   ->samplers[g_tex.sampler_idx_]              //
                                                   .minFilter.value_or(fgltf::Filter::Linear)) //
                               == 1000.0f;
            if (pixels)
            {
                g_tex.data_.resize(g_tex.x_ * g_tex.y_ * g_tex.comp_);
                memcpy(g_tex.data_.data(), pixels, g_tex.data_.size());
                stbi_image_free(pixels);
            }
        }
    };

    auto load_mat_func = [&]()
    {
        materials_.reserve(asset_->materials.size());
        mat_names_.reserve(asset_->materials.size());
        for (const auto& mat : asset_->materials)
        {
            mat_names_.emplace_back(mat.name);
            gltf_mat& g_mat = materials_.emplace_back();

            const fgltf::PBRData& pbr = mat.pbrData;
            glms::assign_value(g_mat.color_factor_, pbr.baseColorFactor);
            g_mat.metallic_factor_ = pbr.metallicFactor;
            g_mat.roughness_factor_ = pbr.roughnessFactor;
            g_mat.color_ = pbr.baseColorTexture ? pbr.baseColorTexture->textureIndex : -1;
            g_mat.metallic_roughness_ = pbr.metallicRoughnessTexture ? pbr.metallicRoughnessTexture->textureIndex : -1;
            g_mat.normal_ = mat.normalTexture ? mat.normalTexture->textureIndex : -1;
            g_mat.occlusion_ = mat.occlusionTexture ? mat.occlusionTexture->textureIndex : -1;
            g_mat.emissive_ = mat.emissiveTexture ? mat.emissiveTexture->textureIndex : -1;
            glms::assign_value(g_mat.emissive_factor_, mat.emissiveFactor);
            switch (mat.alphaMode)
            {
                case fgltf::AlphaMode::Opaque:
                    g_mat.alpha_cutoff_ = 0;
                    break;
                case fgltf::AlphaMode::Mask:
                    g_mat.alpha_cutoff_ = 1;
                    break;
                case fgltf::AlphaMode::Blend:
                    g_mat.alpha_cutoff_ = mat.alphaCutoff;
                    break;
            }
            g_mat.emissive_factor_[3] = mat.emissiveStrength;

            if (mat.anisotropy)
            {
                g_mat.anisotropy_ = mat.anisotropy->anisotropyTexture //
                                        ? mat.anisotropy->anisotropyTexture->textureIndex
                                        : -1;
                g_mat.anisotropy_rotation_ = mat.anisotropy->anisotropyRotation;
                g_mat.anisotropy_strength_ = mat.anisotropy->anisotropyStrength;
            }

            if (mat.sheen)
            {
                g_mat.sheen_color_ = mat.sheen->sheenColorTexture ? mat.sheen->sheenColorTexture->textureIndex : -1;
                g_mat.sheen_roughness_ = mat.sheen->sheenRoughnessTexture //
                                             ? mat.sheen->sheenRoughnessTexture->textureIndex
                                             : -1;
                glms::assign_value(g_mat.sheen_color_factor_, mat.sheen->sheenColorFactor);
            }

            if (mat.specular)
            {
                g_mat.specular_ = mat.specular->specularColorTexture //
                                      ? mat.specular->specularColorTexture->textureIndex
                                      : -1;
                g_mat.spec_color_ = mat.specular->specularColorTexture //
                                        ? mat.specular->specularColorTexture->textureIndex
                                        : -1;
                glms::assign_value(g_mat.specular_color_factor_, mat.specular->specularColorFactor);
                g_mat.specular_color_factor_[3] = mat.specular->specularFactor;
            }
        }
    };

    meshes_.resize(asset_->meshes.size());
    std::vector<std::function<void()>> load_prim_funcs;

    size_t m = 0;
    for (const auto& mesh : asset_->meshes)
    {
        gltf_mesh& g_mesh = meshes_[m];
        g_mesh.name_ = mesh.name;
        g_mesh.prims_.reserve(mesh.primitives.size());
        g_mesh.draw_calls_.reserve(mesh.primitives.size());

        size_t p = 0;
        for (const auto& prim : mesh.primitives)
        {
            prim_count_++;
            gltf_prim* g_prim = &g_mesh.prims_.emplace_back();
            g_prim->mesh_ = m;
            g_prim->name_ = std::format("{}__prim({})", mesh.name, p);
            g_prim->material_ = prim.materialIndex.value_or(0);
            vk::DrawIndirectCommand* draw_call = &g_mesh.draw_calls_.emplace_back();

            load_prim_funcs.emplace_back(
                [&, mesh, m, p, g_prim, draw_call]()
                {
                    const fgltf::Accessor& idx_acc = asset_->accessors[prim.indicesAccessor.value()];
                    g_prim->idxs_.resize(idx_acc.count);
                    fgltf::iterateAccessorWithIndex<uint32_t>(*asset_.get(), idx_acc, //
                                                              [&](uint32_t idx, size_t index)
                                                              { g_prim->idxs_[index] = idx; });

                    check_attribute<glm::vec3>("POSITION", prim, *asset_, g_prim->positions_);
                    check_attribute<glm::vec3>("NORMAL", prim, *asset_, g_prim->normals_);
                    check_attribute<glm::vec4>("TANGENT", prim, *asset_, g_prim->tangents_);
                    check_attribute<glm::vec2>("TEXCOORD_0", prim, *asset_, g_prim->texcoords_);
                    check_attribute<glm::vec4>("COLOR_0", prim, *asset_, g_prim->colors_);
                    check_attribute<glm::uvec4>("JOINTS_0", prim, *asset_, g_prim->joints_);
                    check_attribute<glm::vec4>("WEIGHTS_0", prim, *asset_, g_prim->weights_);

                    for (auto& target : prim.targets)
                    {
                        for (auto& attrib : target)
                        {
                            if (attrib.name == "POSITION")
                            {
                                g_prim->morph_.position_count_++;
                            }
                            else if (attrib.name == "NORMAL")
                            {
                                g_prim->morph_.normal_count_++;
                            }
                            else if (attrib.name == "TANGENT")
                            {
                                g_prim->morph_.tangent_count_++;
                            }
                        }
                    }

                    for (size_t t = 0; t < prim.targets.size(); t++)
                    {
                        for (auto& attrib : prim.targets[t])
                        {
                            const fi::res::fgltf::Accessor& acc = asset_->accessors[attrib.accessorIndex];
                            if (attrib.name == "POSITION")
                            {
                                if (g_prim->morph_.positions_.size() < acc.count * g_prim->morph_.position_count_)
                                {
                                    g_prim->morph_.positions_.resize(acc.count * g_prim->morph_.position_count_);
                                }
                                fi::res::fgltf::iterateAccessorWithIndex<glm::vec3>(
                                    *asset_, acc, //
                                    [&](const glm::vec3& elm, size_t index)
                                    { g_prim->morph_.positions_[index * g_prim->morph_.position_count_ + t] = elm; });
                            }
                            else if (attrib.name == "NORMAL")
                            {
                                if (g_prim->morph_.normals_.size() < acc.count * g_prim->morph_.normal_count_)
                                {
                                    g_prim->morph_.normals_.resize(acc.count * g_prim->morph_.normal_count_);
                                }
                                fi::res::fgltf::iterateAccessorWithIndex<glm::vec3>(
                                    *asset_, acc, //
                                    [&](const glm::vec3& elm, size_t index)
                                    { g_prim->morph_.normals_[index * g_prim->morph_.normal_count_ + t] = elm; });
                            }
                            else if (attrib.name == "TANGENT")
                            {
                                if (g_prim->morph_.tangents_.size() < acc.count * g_prim->morph_.tangent_count_)
                                {
                                    g_prim->morph_.tangents_.resize(acc.count * g_prim->morph_.tangent_count_);
                                }
                                fi::res::fgltf::iterateAccessorWithIndex<glm::vec3>(
                                    *asset_, acc, //
                                    [&](const glm::vec3& elm, size_t index)
                                    { g_prim->morph_.tangents_[index * g_prim->morph_.tangent_count_ + t] = elm; });
                            }
                        }
                    }

                    draw_call->instanceCount = 1;
                    draw_call->vertexCount = idx_acc.count;
                });
            p++;
        }
        m++;
    }

    if (th_pool)
    {
        futs->reserve(futs->size() + 2 + load_prim_funcs.size());
        futs->emplace_back(th_pool->submit(load_tex_func));
        futs->emplace_back(th_pool->submit(load_mat_func));
        for (const auto& func : load_prim_funcs)
        {
            futs->emplace_back(th_pool->submit(func));
        }
    }
    else
    {
        load_tex_func();
        load_mat_func();
        std::for_each(load_prim_funcs.begin(), load_prim_funcs.end(), [](std::function<void()>& func) { func(); });
    }
}