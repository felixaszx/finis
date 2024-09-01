#define STB_IMAGE_IMPLEMENTATION
#include "resources/gltf_file.hpp"

fi::res::gltf_file::gltf_file(const std::filesystem::path& path,
                              std::vector<std::future<void>>* futs,
                              thp::task_thread_pool* th_pool)
{
    fgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None)
    {
        throw std::runtime_error(std::format("Fail to load gltf {}", path.string()));
    }

    auto asset_in = parser.loadGltf(data.get(), path.parent_path(),
                                    fastgltf::Options::LoadExternalBuffers |    //
                                        fastgltf::Options::LoadExternalImages | //
                                        fastgltf::Options::GenerateMeshIndices);
    if (auto error = asset_in.error(); error != fastgltf::Error::None)
    {
        throw std::runtime_error(std::format("Fail to parse gltf {}", path.string()));
    }

    if (auto error = fgltf::validate(asset_in.get()); error != fastgltf::Error::None)
    {
        throw std::runtime_error(std::format("gltf {} is not valid", path.string()));
    }
    make_unique2(asset_, std::move(asset_in.get()));
    name_ = path.filename().generic_string();

    auto load_tex_func = [&]()
    {
        textures_.reserve(asset_->textures.size());
        for (const auto& tex : asset_->textures)
        {
            const fgltf::Image& img = asset_->images[tex.imageIndex.value()];
            const fgltf::BufferView& view = asset_->bufferViews[std::get<1>(img.data).bufferViewIndex];
            const fgltf::Buffer& buffer = asset_->buffers[view.bufferIndex];

            gltf_tex& g_tex = textures_.emplace_back();
            g_tex.name_ = std::format("{}_image({})", tex.name, img.name);
            stbi_uc* pixels =
                stbi_load_from_memory(castf(const stbi_uc*, std::get<3>(buffer.data).bytes.data() + view.byteOffset),
                                      view.byteLength, //
                                      &g_tex.x_, &g_tex.y_, &g_tex.comp_, STBI_rgb_alpha);
            g_tex.comp_ = 4;
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
    std::vector<std::function<void()>> load_mesh_funcs;
    load_mesh_funcs.reserve(meshes_.size());

    size_t m = 0;
    for (const auto& mesh : asset_->meshes)
    {
        load_mesh_funcs.emplace_back(
            [&, mesh, m]()
            {
                gltf_mesh& g_mesh = meshes_[m];
                g_mesh.name_ = mesh.name;
                g_mesh.prims_.reserve(mesh.primitives.size());

                size_t p = 0;
                for (const auto& prim : mesh.primitives)
                {
                    gltf_prim& g_prim = g_mesh.prims_.emplace_back();
                    g_prim.mesh_ = m;
                    g_prim.name_ = std::format("{}_prim({})", mesh.name, p);
                    p++;
                }
            });
        m++;
    }

    if (th_pool)
    {
        futs->reserve(futs->size() + 2 + load_mesh_funcs.size());
        futs->emplace_back(th_pool->submit(load_tex_func));
        futs->emplace_back(th_pool->submit(load_mat_func));
        for (const auto& func : load_mesh_funcs)
        {
            futs->emplace_back(th_pool->submit(func));
        }
    }
    else
    {
        load_tex_func();
        load_mat_func();
        std::for_each(load_mesh_funcs.begin(), load_mesh_funcs.end(), [](std::function<void()>& func) { func(); });
    }
}