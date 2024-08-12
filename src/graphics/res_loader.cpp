#define STB_IMAGE_IMPLEMENTATION
#include "graphics/res_loader.hpp"

fi::ResDetails::ResDetails()
{
    for (TexIdx t = 0; t < tex_imgs_.size(); t++)
    {
        allocator().destroyImage(tex_imgs_[t], tex_allocs_[t]);
        device().destroyImageView(tex_views_[t]);
    }

    for (vk::Sampler sampler : samplers_)
    {
        device().destroySampler(sampler);
    }
}

void fi::ResDetails::add_gltf_file(const std::filesystem::path& path)
{
    if (locked_)
    {
        return;
    }

    // load gltf
    gltf_file_.emplace_back(fastgltf::GltfDataBuffer::FromPath(path));
    if (!gltf_file_.back())
    {
        throw std::runtime_error(std::format("Fail to load {}, Error code {}\n", //
                                             path.generic_string(),              //
                                             gltf::getErrorMessage(gltf_file_.back().error())));
        gltf_file_.pop_back();
        return;
    }

    gltf::Parser parser(gltf::Extensions::EXT_mesh_gpu_instancing |         //
                        gltf::Extensions::KHR_materials_anisotropy |        //
                        gltf::Extensions::KHR_materials_emissive_strength | //
                        gltf::Extensions::KHR_materials_ior |               //
                        gltf::Extensions::KHR_materials_sheen |             //
                        gltf::Extensions::KHR_materials_specular);
    gltf_.emplace_back(parser.loadGltf(gltf_file_.back().get(),                   //
                                       path.parent_path(),                        //
                                       gltf::Options::LoadExternalBuffers |       //
                                           gltf::Options::LoadExternalImages |    //
                                           gltf::Options::DecomposeNodeMatrices | //
                                           gltf::Options::GenerateMeshIndices));
    if (!gltf_.back())
    {
        throw std::runtime_error(std::format("Fail to parse {}, Error code {}\n", //
                                             path.generic_string(),               //
                                             gltf::getErrorMessage(gltf_.back().error())));
        gltf_.pop_back();
        return;
    }

    // load updated data
    gltf::Asset& gltf = gltf_.back().get();
    first_tex_.emplace_back(tex_imgs_.size());
    first_sampler_.emplace_back(samplers_.size());
    first_material_.emplace_back(materials_.size());
    first_mesh_.emplace_back(meshes_.size());
    first_prim_.emplace_back(primitives_.size());

    tex_imgs_.resize(tex_imgs_.size() + gltf.textures.size());
    tex_views_.resize(tex_imgs_.size());
    tex_allocs_.resize(tex_imgs_.size());
    tex_infos_.resize(tex_imgs_.size());
    samplers_.resize(samplers_.size() + gltf.samplers.size());
    meshes_.resize(meshes_.size() + gltf.meshes.size());
    materials_.resize(materials_.size() + gltf.materials.size());

    // load geometric data
    for (TSMeshIdx m_in(0); m_in < gltf.meshes.size(); m_in++)
    {
        // prepare draw call first
        TSMeshIdx m(m_in + first_mesh_.back());
        draw_calls_.reserve(draw_calls_.size() + gltf.meshes[m_in].primitives.size());
        primitives_.reserve(primitives_.size() + gltf.meshes[m_in].primitives.size());
        for (const gltf::Primitive& prim : gltf.meshes[m_in].primitives)
        {
            const gltf::Accessor& pos_acc = gltf.accessors[prim.findAttribute("POSITION")->accessorIndex];
            const gltf::Accessor& idx_acc = gltf.accessors[prim.indicesAccessor.value()];
            vk::DrawIndexedIndirectCommand& draw_call = draw_calls_.emplace_back();
            draw_call.firstIndex = old_idx_count_;
            draw_call.indexCount = idx_acc.count;
            draw_call.firstInstance = 0;
            draw_call.instanceCount = 1;

            const fastgltf::Attribute* normals_attrib = prim.findAttribute("NORMAL");
            const fastgltf::Attribute* tangents_attrib = prim.findAttribute("TANGENT");
            const fastgltf::Attribute* texcoord_attrib = prim.findAttribute("TEXCOORD_0");
            const fastgltf::Attribute* colors_attrib = prim.findAttribute("COLOR_0");
            const fastgltf::Attribute* joints_attrib = prim.findAttribute("JOINTS_0");
            const fastgltf::Attribute* weights_attrib = prim.findAttribute("WEIGHTS_0");
            PrimInfo& prim_info = primitives_.emplace_back();
            prim_info.mesh_idx_ = m;
            prim_info.material_ = first_material_.back() + (uint32_t)prim.materialIndex.value_or(0);
            prim_info.first_position_ = old_vtx_count_;
            if (normals_attrib != prim.attributes.end())
            {
                prim_info.first_normal_ = old_normals_count_;
                old_normals_count_ += gltf.accessors[normals_attrib->accessorIndex].count;
            }
            if (tangents_attrib != prim.attributes.end())
            {
                prim_info.first_tangent_ = old_tangents_count_;
                old_tangents_count_ += gltf.accessors[tangents_attrib->accessorIndex].count;
            }
            if (texcoord_attrib != prim.attributes.end())
            {
                prim_info.first_texcoord_ = old_texcoords_count_;
                old_texcoords_count_ += gltf.accessors[texcoord_attrib->accessorIndex].count;
            }
            if (colors_attrib != prim.attributes.end())
            {
                prim_info.first_color_ = old_colors_count_;
                old_colors_count_ += gltf.accessors[colors_attrib->accessorIndex].count;
            }
            if (joints_attrib != prim.attributes.end())
            {
                prim_info.first_joint_ = old_joints_count_;
                old_joints_count_ += gltf.accessors[joints_attrib->accessorIndex].count;
            }
            if (weights_attrib != prim.attributes.end())
            {
                prim_info.first_weight_ = old_weights_count_;
                old_weights_count_ += gltf.accessors[weights_attrib->accessorIndex].count;
            }

            old_vtx_count_ += pos_acc.count;
            old_idx_count_ += idx_acc.count;
        }
    }

    vtx_positions_.resize(old_vtx_count_, {0, 0, 0});
    idxs_.resize(old_idx_count_);
    vtx_normals_.resize(old_normals_count_);
    vtx_tangents_.resize(old_tangents_count_);
    vtx_texcoords_.resize(old_texcoords_count_);
    vtx_colors_.resize(old_colors_count_);
    vtx_joints_.resize(old_joints_count_);
    vtx_weights_.resize(old_weights_count_);
}
void fi::ResDetails::lock_and_load()
{
    locked_ = true; // load geometric data
    std::vector<std::future<void>> futs;
    for (size_t g = 0; g < gltf_.size(); g++)
    {
        gltf::Asset* gltf = &gltf_[g].get();
        TSTexIdx first_tex = first_tex_[g];
        TSMaterialIdx first_material = first_material_[g];
        TSMeshIdx first_mesh = first_mesh_[g];
        PrimIdx first_prim = first_prim_[g];

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto draw_call_iter = draw_calls_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const auto& accessor = gltf->accessors[prim.indicesAccessor.value()];
                        fastgltf::iterateAccessorWithIndex<uint32_t>(
                            *gltf, accessor, //
                            [&](uint32_t idx, size_t iter) { idxs_[draw_call_iter->firstIndex + iter] = idx; });
                    }
                    draw_call_iter++;
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const gltf::Accessor& pos_acc = gltf->accessors[prim.findAttribute("POSITION")->accessorIndex];
                        fastgltf::iterateAccessorWithIndex<gltf::math::fvec3>(
                            *gltf, pos_acc, //
                            [&](const gltf::math::fvec3& position, size_t iter)
                            { glms::assign_value(vtx_positions_[prim_info->first_position_ + iter], position); });
                    }
                    prim_info++;
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fastgltf::Attribute* normals_attrib = prim.findAttribute("NORMAL");
                        if (normals_attrib != prim.attributes.end())
                        {
                            const gltf::Accessor& acc = gltf->accessors[normals_attrib->accessorIndex];
                            fastgltf::iterateAccessorWithIndex<gltf::math::fvec3>(
                                *gltf, acc, //
                                [&](const gltf::math::fvec3& normal, size_t iter)
                                { glms::assign_value(vtx_normals_[prim_info->first_normal_ + iter], normal); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fastgltf::Attribute* tangents_attrib = prim.findAttribute("TANGENT");
                        if (tangents_attrib != prim.attributes.end())
                        {
                            const gltf::Accessor& acc = gltf->accessors[tangents_attrib->accessorIndex];
                            fastgltf::iterateAccessorWithIndex<gltf::math::fvec4>(
                                *gltf, acc, //
                                [&](const gltf::math::fvec4& tangent, size_t iter)
                                { glms::assign_value(vtx_tangents_[prim_info->first_tangent_ + iter], tangent); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fastgltf::Attribute* texcoord_attrib = prim.findAttribute("TEXCOORD_0");
                        if (texcoord_attrib != prim.attributes.end())
                        {
                            const gltf::Accessor& acc = gltf->accessors[texcoord_attrib->accessorIndex];
                            fastgltf::iterateAccessorWithIndex<gltf::math::fvec2>(
                                *gltf, acc, //
                                [&](const gltf::math::fvec2& tex_coord, size_t iter)
                                { glms::assign_value(vtx_texcoords_[prim_info->first_texcoord_ + iter], tex_coord); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fastgltf::Attribute* colors_attrib = prim.findAttribute("COLOR_0");
                        if (colors_attrib != prim.attributes.end())
                        {
                            const gltf::Accessor& acc = gltf->accessors[colors_attrib->accessorIndex];
                            fastgltf::iterateAccessorWithIndex<gltf::math::fvec4>(
                                *gltf, acc, //
                                [&](const gltf::math::fvec4& color, size_t iter)
                                { glms::assign_value(vtx_colors_[prim_info->first_color_ + iter], color); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fastgltf::Attribute* joints_attrib = prim.findAttribute("JOINTS_0");
                        if (joints_attrib != prim.attributes.end())
                        {
                            const gltf::Accessor& acc = gltf->accessors[joints_attrib->accessorIndex];
                            fastgltf::iterateAccessorWithIndex<gltf::math::uvec4>(
                                *gltf, acc, //
                                [&](const gltf::math::uvec4& joints, size_t iter)
                                { glms::assign_value(vtx_joints_[prim_info->first_joint_ + iter], joints); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const gltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fastgltf::Attribute* weights_attrib = prim.findAttribute("WEIGHTS_0");
                        if (weights_attrib != prim.attributes.end())
                        {
                            const gltf::Accessor& acc = gltf->accessors[weights_attrib->accessorIndex];
                            fastgltf::iterateAccessorWithIndex<gltf::math::fvec4>(
                                *gltf, acc, //
                                [&](const gltf::math::fvec4& weights, size_t iter)
                                { glms::assign_value(vtx_weights_[prim_info->first_weight_ + iter], weights); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_tex, first_material]()
            {
                for (MaterialIdx m(0); m < gltf->materials.size(); m++)
                {
                    MaterialInfo& material = materials_[first_material + m];
                    const gltf::Material& mat_in = gltf->materials[m];
                    const gltf::PBRData& pbr = mat_in.pbrData;

                    glms::assign_value(material.color_factor_, pbr.baseColorFactor);
                    material.metalic_factor_ = pbr.metallicFactor;
                    material.roughtness_factor_ = pbr.roughnessFactor;
                    material.color_ = pbr.baseColorTexture ? first_tex + pbr.baseColorTexture->textureIndex : EMPTY;
                    material.metalic_roughtness_ = pbr.metallicRoughnessTexture ? //
                                                       first_tex + pbr.metallicRoughnessTexture->textureIndex
                                                                                : EMPTY;

                    material.normal_ = mat_in.normalTexture ? first_tex + mat_in.normalTexture->textureIndex : EMPTY;
                    material.occlusion_ = mat_in.occlusionTexture ? //
                                              first_tex + mat_in.occlusionTexture->textureIndex
                                                                  : EMPTY;

                    material.emissive_factor_[3] = mat_in.emissiveStrength;
                    glms::assign_value(material.emissive_factor_, mat_in.emissiveFactor, 3);
                    material.emissive_ = mat_in.emissiveTexture ? //
                                             first_tex + mat_in.emissiveTexture->textureIndex
                                                                : EMPTY;

                    switch (mat_in.alphaMode)
                    {
                        case gltf::AlphaMode::Opaque:
                        {
                            material.alpha_cutoff_ = 0;
                            break;
                        }
                        case gltf::AlphaMode::Blend:
                        {
                            material.alpha_cutoff_ = -1;
                            break;
                        }
                        case gltf::AlphaMode::Mask:
                        {
                            material.alpha_cutoff_ = mat_in.alphaCutoff;
                            break;
                        }
                    }

                    const auto& anistropy = mat_in.anisotropy;
                    if (anistropy)
                    {
                        material.anisotropy_ = anistropy->anisotropyTexture
                                                   ? first_tex + anistropy->anisotropyTexture->textureIndex
                                                   : EMPTY;
                        material.anisotropy_rotation_ = anistropy->anisotropyRotation;
                        material.anisotropy_strength_ = anistropy->anisotropyStrength;
                    }

                    const auto& sheen = mat_in.sheen;
                    if (sheen)
                    {
                        glms::assign_value(material.sheen_color_factor_, sheen->sheenColorFactor, 3);
                        material.sheen_color_factor_[3] = sheen->sheenRoughnessFactor;
                        material.sheen_color_ = sheen->sheenColorTexture //
                                                    ? first_tex + sheen->sheenColorTexture->textureIndex
                                                    : EMPTY;
                        material.sheen_roughtness_ = sheen->sheenRoughnessTexture //
                                                         ? first_tex + sheen->sheenRoughnessTexture->textureIndex
                                                         : EMPTY;
                    }

                    const auto& specular = mat_in.specular;
                    if (specular)
                    {
                        glms::assign_value(material.specular_color_factor_, specular->specularColorFactor, 3);
                        material.specular_color_factor_[3] = specular->specularFactor;
                        material.specular_ = specular->specularTexture //
                                                 ? first_tex + specular->specularTexture->textureIndex
                                                 : EMPTY;
                        material.spec_color_ = specular->specularColorTexture //
                                                   ? first_tex + specular->specularColorTexture->textureIndex
                                                   : EMPTY;
                    }
                }
            }));
    }

    for (size_t g = 0; g < gltf_.size(); g++)
    {
        gltf::Asset* gltf = &gltf_[g].get();
        TSTexIdx first_tex = first_tex_[g];
        TSSamplerIdx first_sampler = first_sampler_[g];

        futs.emplace_back(th_pool_.submit_task(
            [this, gltf, first_tex, first_sampler]()
            {
                vk::CommandPoolCreateInfo tex_cmd_pool_info{vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                            queue_indices(GRAPHICS)};
                vk::CommandPool tex_cmd_pool = device().createCommandPool(tex_cmd_pool_info);
                vk::CommandBufferAllocateInfo cmd_alloc_info{tex_cmd_pool, {}, 2};
                vk::CommandBuffer tex_cmd = device().allocateCommandBuffers(cmd_alloc_info)[0];
                Fence upload_completed(false);

                auto extract_mipmaped = [](gltf::Filter filter)
                {
                    switch (filter)
                    {
                        case gltf::Filter::NearestMipMapNearest:
                        case gltf::Filter::LinearMipMapNearest:
                        case gltf::Filter::NearestMipMapLinear:
                        case gltf::Filter::LinearMipMapLinear:
                            return true;
                        default:
                            return false;
                    }
                };

                for (TSSamplerIdx s_g(0); s_g < gltf->samplers.size(); s_g++)
                {
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

                    auto extract_wrapping = [](gltf::Wrap wrap)
                    {
                        switch (wrap)
                        {
                            case gltf::Wrap::ClampToEdge:
                                return vk::SamplerAddressMode::eClampToEdge;
                            case gltf::Wrap::MirroredRepeat:
                                return vk::SamplerAddressMode::eMirroredRepeat;
                            default:
                            case gltf::Wrap::Repeat:
                                return vk::SamplerAddressMode::eRepeat;
                        }
                    };

                    const gltf::Sampler& sampler = gltf->samplers[s_g];
                    TSSamplerIdx s(first_sampler + s_g);
                    vk::SamplerCreateInfo sampler_info{};
                    sampler_info.addressModeU = extract_wrapping(sampler.wrapS);
                    sampler_info.addressModeV = extract_wrapping(sampler.wrapT);
                    sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
                    sampler_info.mipmapMode =
                        extract_mipmap_mode(sampler.minFilter.value_or(gltf::Filter::LinearMipMapLinear));
                    sampler_info.minFilter =
                        extract_filter(sampler.minFilter.value_or(gltf::Filter::LinearMipMapLinear));
                    sampler_info.magFilter =
                        extract_filter(sampler.magFilter.value_or(gltf::Filter::LinearMipMapLinear));
                    sampler_info.maxLod =
                        extract_mipmaped(sampler.minFilter.value_or(gltf::Filter::LinearMipMapLinear)) //
                            ? VK_LOD_CLAMP_NONE
                            : 0;
                    samplers_[s] = device().createSampler(sampler_info);
                }

                for (TSTexIdx t_g(0); t_g < gltf->textures.size(); t_g++)
                {
                    const gltf::Image& img_g = gltf->images[gltf->textures[t_g].imageIndex.value()];
                    const gltf::BufferView& img_bufer_view = gltf->bufferViews[std::get<1>(img_g.data).bufferViewIndex];
                    const gltf::Buffer& img_bufer = gltf->buffers[img_bufer_view.bufferIndex];
                    int w = 0, h = 0, c = 0;
                    stbi_uc* pixels = stbi_load_from_memory(
                        (const unsigned char*)std::get<3>(img_bufer.data).bytes.data() + img_bufer_view.byteOffset,
                        img_bufer_view.byteLength, //
                        &w, &h, &c,                //
                        STBI_rgb_alpha);

                    bool mip_mapping = extract_mipmaped(gltf->samplers[gltf->textures[t_g].samplerIndex.value_or(0)] //
                                                            .minFilter.value_or(gltf::Filter::LinearMipMapLinear));
                    uint32_t levels = mip_mapping ? std::floor(std::log2(std::max(w, h))) + 1 : 1;

                    TSTexIdx tex(first_tex + t_g);
                    {
                        vma::AllocationCreateInfo alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
                        vk::ImageCreateInfo img_info{{},
                                                     vk::ImageType::e2D,
                                                     vk::Format::eR8G8B8A8Srgb,
                                                     vk::Extent3D(w, h, 1),
                                                     levels,
                                                     1,
                                                     vk::SampleCountFlagBits::e1,
                                                     vk::ImageTiling::eOptimal,
                                                     vk::ImageUsageFlagBits::eSampled |
                                                         vk::ImageUsageFlagBits::eTransferDst |
                                                         vk::ImageUsageFlagBits::eTransferSrc};
                        auto allocated = allocator().createImage(img_info, alloc_info);
                        tex_imgs_[tex] = allocated.first;
                        tex_allocs_[tex] = allocated.second;

                        vk::ImageViewCreateInfo view_info{
                            {},
                            tex_imgs_[tex],
                            vk::ImageViewType::e2D,
                            img_info.format,
                            {},
                            {vk::ImageAspectFlagBits::eColor, 0, img_info.mipLevels, 0, img_info.arrayLayers}};
                        tex_views_[tex] = device().createImageView(view_info);
                    }

                    Buffer<BufferBase::EmptyExtraInfo, seq_write, host_coherent, vertex> //
                        staging(img_bufer_view.byteLength, SRC);
                    memcpy(staging.map_memory(), pixels, img_bufer_view.byteLength);
                    stbi_image_free(pixels);

                    vk::ImageMemoryBarrier barrier{};
                    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = tex_imgs_[tex];
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
                    region.imageExtent = vk::Extent3D(w, h, 1);

                    tex_cmd.reset();
                    begin_cmd(tex_cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
                    tex_cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                            vk::PipelineStageFlagBits::eTransfer, //
                                            {}, {}, {}, barrier);
                    tex_cmd.copyBufferToImage(staging, barrier.image, vk::ImageLayout::eTransferDstOptimal, region);

                    int32_t mip_w = w;
                    int32_t mip_h = h;
                    for (int i = 1; i < levels; i++)
                    {
                        barrier.subresourceRange.levelCount = 1;
                        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
                        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
                        barrier.subresourceRange.baseMipLevel = i - 1;
                        tex_cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                vk::PipelineStageFlagBits::eTransfer, //
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
                        tex_cmd.blitImage(tex_imgs_[tex], vk::ImageLayout::eTransferSrcOptimal, //
                                          tex_imgs_[tex], vk::ImageLayout::eTransferDstOptimal, //
                                          blit, vk::Filter::eLinear);

                        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
                        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                        barrier.subresourceRange.baseMipLevel = i - 1;
                        tex_cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                vk::PipelineStageFlagBits::eTransfer, //
                                                {}, {}, {}, barrier);

                        mip_w > 1 ? mip_w /= 2 : mip_w;
                        mip_h > 1 ? mip_h /= 2 : mip_h;
                    }
                    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                    barrier.subresourceRange.baseMipLevel = levels - 1;
                    tex_cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                            vk::PipelineStageFlagBits::eTransfer, //
                                            {}, {}, {}, barrier);
                    tex_cmd.end();

                    vk::SubmitInfo submit{};
                    submit.setCommandBuffers(tex_cmd);
                    queues(GRAPHICS).submit(submit, upload_completed);

                    vk::DescriptorImageInfo& tex_info = tex_infos_[tex];
                    tex_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                    tex_info.imageView = tex_views_[tex];
                    tex_info.sampler = samplers_[first_sampler + gltf->textures[t_g].samplerIndex.value_or(0)];
                    auto r = device().waitForFences(upload_completed, true, std::numeric_limits<uint64_t>::max());
                    device().resetFences(upload_completed);
                }

                device().destroyCommandPool(tex_cmd_pool);
            }));
    }

    for (const std::future<void>& fut : futs)
    {
        fut.wait();
    }
}
