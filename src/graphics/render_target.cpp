#include "graphics/graphics.hpp"
#include "graphics/render_target.hpp"

fi::SceneResources::~SceneResources()
{
    for (auto sampler : samplers_)
    {
        device().destroySampler(sampler);
    }
}

std::array<vk::VertexInputBindingDescription, 2> fi::GltfLoader::vertex_bindings()
{
    std::array<vk::VertexInputBindingDescription, 2> bindings({});

    bindings[0].binding = 0;
    bindings[0].stride = sizeof(Vertex);
    bindings[0].inputRate = vk::VertexInputRate::eVertex;

    bindings[1].binding = 1;
    bindings[1].stride = sizeof(glm::mat4);
    bindings[1].inputRate = vk::VertexInputRate::eInstance;

    return bindings;
}

std::array<vk::VertexInputAttributeDescription, 9> fi::GltfLoader::vertex_attributes()
{
    std::array<vk::VertexInputAttributeDescription, 9> attributes({});

    attributes[0].format = vk::Format::eR32G32B32Sfloat;
    attributes[1].format = vk::Format::eR32G32B32Sfloat;
    attributes[2].format = vk::Format::eR32G32Sfloat;
    attributes[3].format = vk::Format::eR32G32B32A32Sfloat;
    attributes[4].format = vk::Format::eR32G32B32A32Sfloat;

    attributes[0].offset = offsetof(Vertex, position_);
    attributes[1].offset = offsetof(Vertex, normal_);
    attributes[2].offset = offsetof(Vertex, tex_coord_);
    attributes[3].offset = offsetof(Vertex, bone_ids_);
    attributes[4].offset = offsetof(Vertex, bone_weight_);

    for (uint32_t i = 5; i < 9; i++)
    {
        attributes[i].binding = 1;
        attributes[i].location = i;
        attributes[i].format = vk::Format::eR32G32B32A32Sfloat;
        attributes[i].offset = (i - 5) * sizeof(glm::vec4);
    }

    return attributes;
}

std::vector<fi::RenderableScene> fi::GltfLoader::from_file(const std::filesystem::path& path, ImageMgr& img_mgr)
{
    auto data = gltf::GltfDataBuffer::FromPath(path);
    auto asset = gltf_parser().loadGltf(data.get(), path.parent_path(),
                                        gltf::Options::GenerateMeshIndices | //
                                            gltf::Options::LoadExternalImages);
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
        auto decode_adress_mode = [](gltf::Wrap wrap)
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

        auto filter = [](gltf::Filter filter)
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

        auto mipmap_mode = [](gltf::Filter filter)
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
        sampler_info.borderColor = vk::BorderColor::eFloatOpaqueBlack;
        sampler_info.maxLod = 1000.0f;
        sampler_info.mipmapMode = mipmap_mode(sampler.magFilter.value_or(gltf::Filter::Linear));
        sampler_info.magFilter = filter(sampler.magFilter.value_or(gltf::Filter::Linear));
        sampler_info.minFilter = filter(sampler.minFilter.value_or(gltf::Filter::Linear));
        shared_res->samplers_.push_back(device().createSampler(sampler_info));
    }

    // load all textures
    std::vector<Image*> images; // use later
    images.reserve(asset->images.size());
    for (auto& img : asset->images)
    {
        if (img.data.index())
        {
            gltf::sources::BufferView img_buf = std::get<gltf::sources::BufferView>(img.data);
            size_t data_idx = asset->bufferViews[img_buf.bufferViewIndex].bufferIndex;
            gltf::sources::Array img_byte = std::get<gltf::sources::Array>(asset->buffers[data_idx].data);
            size_t begin = asset->bufferViews[img_buf.bufferViewIndex].byteOffset;
            size_t length = asset->bufferViews[img_buf.bufferViewIndex].byteLength;
            images.push_back(&img_mgr.load_image(img.name.c_str(), img_byte.bytes, begin, length));
        }
    }

    for (auto& tex : asset->textures)
    {
        if (tex.imageIndex)
        {
            images[tex.imageIndex.value()]->sampler_ = shared_res->samplers_[tex.samplerIndex.value()];
        }
    }

    return scenes;
}
