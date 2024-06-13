#ifndef GRAPHICS_RENDER_TARGET_HPP
#define GRAPHICS_RENDER_TARGET_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "glms.hpp"

namespace fi
{
    struct CombinedMaterial
    {
        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};
    };

    struct SceneResources
    {
        std::vector<vk::Sampler> samplers_{};
        std::vector<CombinedMaterial> materials_{};

        ~SceneResources() = default;
    };

    struct RenderableScene
    {
        std::shared_ptr<SceneResources> res_{};
    };

    struct GltfLoader : private GraphicsObject
    {
        std::vector<RenderableScene> from_file(const std::filesystem::path& path, ImageMgr& img_mrg)
        {
            auto data = gltf::GltfDataBuffer::FromPath(path);
            auto asset = gltf_parser().loadGltf(data.get(), path.parent_path(), gltf::Options::GenerateMeshIndices);

            std::vector<RenderableScene> scenes(asset->scenes.size());

            auto shared_res = std::make_shared<SceneResources>();
            for (auto& scene : scenes)
            {
                scene.res_ = shared_res;
            }

            // create all sampelrs
            shared_res->samplers_.reserve(asset->samplers.size());
            for (auto& sampler : asset->samplers)
            {
                auto decode_adress_mode = [&](gltf::Wrap wrap)
                {
                    switch (wrap)
                    {
                        case gltf::Wrap::Repeat:
                            return vk::SamplerAddressMode::eRepeat;
                        case gltf::Wrap::MirroredRepeat:
                            return vk::SamplerAddressMode::eMirroredRepeat;
                        case gltf::Wrap::ClampToEdge:
                            return vk::SamplerAddressMode::eClampToEdge;
                    }
                };

                vk::SamplerCreateInfo sampler_info{};
                sampler_info.anisotropyEnable = true;
                sampler_info.maxAnisotropy = 4;
                sampler_info.borderColor = vk::BorderColor::eFloatOpaqueBlack;
                sampler_info.maxLod = 1000.0f;
                sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
                sampler_info.magFilter = vk::Filter::eLinear;
                sampler_info.minFilter = vk::Filter::eLinear;
                shared_res->samplers_.push_back(device().createSampler(sampler_info));
            }

            // load all textures
            std::vector<Image> images;
            images.reserve(asset->images.size());
            for (auto& img : asset->images)
            {
            }

            for (uint32_t s = 0; s < scenes.size(); s++)
            {
                gltf::iterateSceneNodes(asset.get(), s, gltf::math::fmat4x4(),
                                        [&](gltf::Node& node, const gltf::math::fmat4x4& matrix)
                                        {
                                            if (node.meshIndex.has_value())
                                            {
                                                gltf::Mesh& mesh = asset->meshes[node.meshIndex.value()];
                                                gltf::Material mat;
                                            }
                                        });
            }
            return scenes;
        }
    };

}; // namespace fi

#endif // GRAPHICS_RENDER_TARGET_HPP
