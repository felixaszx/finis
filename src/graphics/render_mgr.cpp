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
    return texture_set_layouts_;
}

std::vector<fi::Renderable> fi::RenderMgr::upload_res(const std::filesystem::path& path, TextureMgr& texture_mgr)
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
    std::vector<vk::DescriptorImageInfo>& texture_info = texture_infos_.emplace_back();
    samplers.reserve(asset->samplers.size());
    texture_info.reserve(asset->textures.size());

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

        Texture& img = texture_mgr.load_texture(tex.name == "" ? asset->images[img_idx].name.c_str() : tex.name.c_str(),
                                                img_data, view.byteOffset, view.byteLength);
        img.sampler_ = samplers[tex.samplerIndex.value_or(0)];
        texture_info.push_back(img);
    }

    materials_.resize(asset->materials.size());

    return {};
}

void fi::RenderMgr::draw(vk::CommandBuffer cmd, const std::vector<Renderable>& renderables,
                         const std::function<void(vk::DescriptorSet texture_set)>& prepare)
{
    if (!locked_)
    {
        return;
    }
    prepare(texture_sets_[renderables[0].data_idx_]);
    cmd.drawIndexedIndirect(host_buffers_[renderables[0].data_idx_], 0,     //
                            host_buffers_[renderables[0].data_idx_].size(), //
                            sizeof(vk::DrawIndexedIndirectCommand));
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
        texture_set_layouts_.push_back(device().createDescriptorSetLayout(layout_info));
    }

    vk::DescriptorPoolCreateInfo pool_info{};
    pool_info.setPoolSizes(sizes);
    pool_info.maxSets = texture_set_layouts_.size();
    pool_ = device().createDescriptorPool(pool_info);

    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = pool_;
    alloc_info.setSetLayouts(texture_set_layouts_);
    texture_sets_ = device().allocateDescriptorSets(alloc_info);
}
