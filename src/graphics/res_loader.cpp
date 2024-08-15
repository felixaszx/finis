#define STB_IMAGE_IMPLEMENTATION
#include "graphics/res_loader.hpp"

fi::ResDetails::~ResDetails()
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

    device().destroyDescriptorSetLayout(set_layout_);
}

void fi::ResDetails::add_gltf_file(const std::filesystem::path& path)
{
    if (locked_)
    {
        return;
    }

    // load gltf
    auto gltf_file = fgltf::GltfDataBuffer::FromPath(path);
    if (!gltf_file)
    {
        throw std::runtime_error(std::format("Fail to load {}, Error code {}\n", //
                                             path.generic_string(),              //
                                             fgltf::getErrorMessage(gltf_file.error())));
        return;
    }

    fgltf::Parser parser(fgltf::Extensions::EXT_mesh_gpu_instancing |         //
                         fgltf::Extensions::KHR_materials_anisotropy |        //
                         fgltf::Extensions::KHR_materials_emissive_strength | //
                         fgltf::Extensions::KHR_materials_ior |               //
                         fgltf::Extensions::KHR_materials_sheen |             //
                         fgltf::Extensions::KHR_materials_specular);
    gltf_.emplace_back(parser.loadGltf(gltf_file.get(),                            //
                                       path.parent_path(),                         //
                                       fgltf::Options::LoadExternalBuffers |       //
                                           fgltf::Options::LoadExternalImages |    //
                                           fgltf::Options::DecomposeNodeMatrices | //
                                           fgltf::Options::GenerateMeshIndices));
    if (!gltf_.back())
    {
        throw std::runtime_error(std::format("Fail to parse {}, Error code {}\n", //
                                             path.generic_string(),               //
                                             fgltf::getErrorMessage(gltf_.back().error())));
        gltf_.pop_back();
        return;
    }

    // load updated data
    fgltf::Asset& gltf = gltf_.back().get();
    first_tex_.emplace_back(tex_imgs_.size());
    first_sampler_.emplace_back(samplers_.size());
    first_material_.emplace_back(materials_.size());
    first_mesh_.emplace_back(meshes_.size());
    first_prim_.emplace_back(primitives_.size());
    first_morph_target_.emplace_back(morph_targets_.size());

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
        for (const fgltf::Primitive& prim : gltf.meshes[m_in].primitives)
        {
            const fgltf::Accessor& pos_acc = gltf.accessors[prim.findAttribute("POSITION")->accessorIndex];
            const fgltf::Accessor& idx_acc = gltf.accessors[prim.indicesAccessor.value()];
            vk::DrawIndexedIndirectCommand& draw_call = draw_calls_.emplace_back();
            draw_call.firstIndex = old_idx_count_;
            draw_call.indexCount = idx_acc.count;
            draw_call.firstInstance = 0;
            draw_call.instanceCount = 1;

            const fgltf::Attribute* normals_attrib = prim.findAttribute("NORMAL");
            const fgltf::Attribute* tangents_attrib = prim.findAttribute("TANGENT");
            const fgltf::Attribute* texcoord_attrib = prim.findAttribute("TEXCOORD_0");
            const fgltf::Attribute* colors_attrib = prim.findAttribute("COLOR_0");
            const fgltf::Attribute* joints_attrib = prim.findAttribute("JOINTS_0");
            const fgltf::Attribute* weights_attrib = prim.findAttribute("WEIGHTS_0");
            PrimInfo& prim_info = primitives_.emplace_back();
            prim_info.mesh_idx_ = m;
            prim_info.material_ = first_material_.back() + (uint32_t)prim.materialIndex.value_or(0);
            prim_info.first_position_ = old_vtx_count_ * 3;
            if (normals_attrib != prim.attributes.end())
            {
                prim_info.first_normal_ = old_normals_count_ * 3;
                old_normals_count_ += gltf.accessors[normals_attrib->accessorIndex].count;
            }
            if (tangents_attrib != prim.attributes.end())
            {
                prim_info.first_tangent_ = old_tangents_count_ * 4;
                old_tangents_count_ += gltf.accessors[tangents_attrib->accessorIndex].count;
            }
            if (texcoord_attrib != prim.attributes.end())
            {
                prim_info.first_texcoord_ = old_texcoords_count_ * 2;
                old_texcoords_count_ += gltf.accessors[texcoord_attrib->accessorIndex].count;
            }
            if (colors_attrib != prim.attributes.end())
            {
                prim_info.first_color_ = old_colors_count_ * 4;
                old_colors_count_ += gltf.accessors[colors_attrib->accessorIndex].count;
            }
            if (joints_attrib != prim.attributes.end())
            {
                prim_info.first_joint_ = old_joints_count_ * 4;
                old_joints_count_ += gltf.accessors[joints_attrib->accessorIndex].count;
            }
            if (weights_attrib != prim.attributes.end())
            {
                prim_info.first_weight_ = old_weights_count_ * 4;
                old_weights_count_ += gltf.accessors[weights_attrib->accessorIndex].count;
            }

            if (!prim.targets.empty())
            {
                prim_info.morph_target_ = morph_targets_.size();
                morph_targets_.emplace_back();
            }

            size_t added_target_positions = 0;
            size_t added_target_nornmal = 0;
            size_t added_target_tangent = 0;
            for (const auto& target : prim.targets)
            {
                for (const auto& attrib : target)
                {
                    if (attrib.name == "POSITION")
                    {
                        morph_targets_[prim_info.morph_target_].position_morph_count_++;
                        added_target_positions += gltf.accessors[attrib.accessorIndex].count;
                    }
                    else if (attrib.name == "NORMAL")
                    {
                        morph_targets_[prim_info.morph_target_].normal_morph_count_++;
                        added_target_nornmal += gltf.accessors[attrib.accessorIndex].count;
                    }
                    else if (attrib.name == "TANGENT")
                    {
                        morph_targets_[prim_info.morph_target_].tangent_morph_count_++;
                        added_target_tangent += gltf.accessors[attrib.accessorIndex].count;
                    }
                }
            }

            if (added_target_positions)
            {
                morph_targets_[prim_info.morph_target_].first_position_ = old_target_positions_count * 3;
                old_target_positions_count += added_target_positions;
            }
            if (added_target_nornmal)
            {
                morph_targets_[prim_info.morph_target_].first_normal_ = old_target_normals_count_ * 3;
                old_target_normals_count_ += added_target_nornmal;
            }
            if (added_target_tangent)
            {
                morph_targets_[prim_info.morph_target_].first_tangent_ = old_target_tangents_count_ * 4;
                old_target_tangents_count_ += added_target_tangent;
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
    target_positions_.resize(old_target_positions_count);
    target_normals_.resize(old_target_normals_count_);
    target_tangents_.resize(old_target_tangents_count_);
}

void fi::ResDetails::lock_and_load()
{
    if (locked_)
    {
        return;
    }
    locked_ = true;
    // threading helpers
    bs::thread_pool th_pool;

    // load geometric data
    std::vector<std::future<void>> futs;
    for (size_t g = 0; g < gltf_.size(); g++)
    {
        fgltf::Asset* gltf = &gltf_[g].get();
        TSTexIdx first_tex = first_tex_[g];
        TSMaterialIdx first_material = first_material_[g];
        TSMeshIdx first_mesh = first_mesh_[g];
        PrimIdx first_prim = first_prim_[g];

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto draw_call_iter = draw_calls_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const auto& accessor = gltf->accessors[prim.indicesAccessor.value()];
                        fgltf::iterateAccessorWithIndex<uint32_t>(*gltf, accessor, //
                                                                  [&](uint32_t idx, size_t iter)
                                                                  { idxs_[draw_call_iter->firstIndex + iter] = idx; });
                        draw_call_iter++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Accessor& pos_acc = gltf->accessors[prim.findAttribute("POSITION")->accessorIndex];
                        fgltf::iterateAccessorWithIndex<fgltf::math::fvec3>(
                            *gltf, pos_acc, //
                            [&](const fgltf::math::fvec3& position, size_t iter)
                            { glms::assign_value(vtx_positions_[prim_info->first_position_ / 3 + iter], position); });
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Attribute* normals_attrib = prim.findAttribute("NORMAL");
                        if (normals_attrib != prim.attributes.end())
                        {
                            const fgltf::Accessor& acc = gltf->accessors[normals_attrib->accessorIndex];
                            fgltf::iterateAccessorWithIndex<fgltf::math::fvec3>(
                                *gltf, acc, //
                                [&](const fgltf::math::fvec3& normal, size_t iter)
                                { glms::assign_value(vtx_normals_[prim_info->first_normal_ / 3 + iter], normal); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Attribute* tangents_attrib = prim.findAttribute("TANGENT");
                        if (tangents_attrib != prim.attributes.end())
                        {
                            const fgltf::Accessor& acc = gltf->accessors[tangents_attrib->accessorIndex];
                            fgltf::iterateAccessorWithIndex<fgltf::math::fvec4>(
                                *gltf, acc, //
                                [&](const fgltf::math::fvec4& tangent, size_t iter)
                                { glms::assign_value(vtx_tangents_[prim_info->first_tangent_ / 4 + iter], tangent); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Attribute* texcoord_attrib = prim.findAttribute("TEXCOORD_0");
                        if (texcoord_attrib != prim.attributes.end())
                        {
                            const fgltf::Accessor& acc = gltf->accessors[texcoord_attrib->accessorIndex];
                            fgltf::iterateAccessorWithIndex<fgltf::math::fvec2>(
                                *gltf, acc, //
                                [&](const fgltf::math::fvec2& tex_coord, size_t iter) {
                                    glms::assign_value(vtx_texcoords_[prim_info->first_texcoord_ / 2 + iter],
                                                       tex_coord);
                                });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Attribute* colors_attrib = prim.findAttribute("COLOR_0");
                        if (colors_attrib != prim.attributes.end())
                        {
                            const fgltf::Accessor& acc = gltf->accessors[colors_attrib->accessorIndex];
                            fgltf::iterateAccessorWithIndex<fgltf::math::fvec4>(
                                *gltf, acc, //
                                [&](const fgltf::math::fvec4& color, size_t iter)
                                { glms::assign_value(vtx_colors_[prim_info->first_color_ / 4 + iter], color); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Attribute* joints_attrib = prim.findAttribute("JOINTS_0");
                        if (joints_attrib != prim.attributes.end())
                        {
                            const fgltf::Accessor& acc = gltf->accessors[joints_attrib->accessorIndex];
                            fgltf::iterateAccessorWithIndex<fgltf::math::uvec4>(
                                *gltf, acc, //
                                [&](const fgltf::math::uvec4& joints, size_t iter)
                                { glms::assign_value(vtx_joints_[prim_info->first_joint_ / 4 + iter], joints); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        const fgltf::Attribute* weights_attrib = prim.findAttribute("WEIGHTS_0");
                        if (weights_attrib != prim.attributes.end())
                        {
                            const fgltf::Accessor& acc = gltf->accessors[weights_attrib->accessorIndex];
                            fgltf::iterateAccessorWithIndex<fgltf::math::fvec4>(
                                *gltf, acc, //
                                [&](const fgltf::math::fvec4& weights, size_t iter)
                                { glms::assign_value(vtx_weights_[prim_info->first_weight_ / 4 + iter], weights); });
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_tex, first_material]()
            {
                for (MaterialIdx m(0); m < gltf->materials.size(); m++)
                {
                    MaterialInfo& material = materials_[first_material + m];
                    const fgltf::Material& mat_in = gltf->materials[m];
                    const fgltf::PBRData& pbr = mat_in.pbrData;

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
                        case fgltf::AlphaMode::Opaque:
                        {
                            material.alpha_cutoff_ = 0;
                            break;
                        }
                        case fgltf::AlphaMode::Blend:
                        {
                            material.alpha_cutoff_ = -1;
                            break;
                        }
                        case fgltf::AlphaMode::Mask:
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

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        size_t target_idx = 0;
                        for (const auto& target : prim.targets)
                        {
                            for (const auto& attrib : target)
                            {
                                if (attrib.name == "POSITION")
                                {
                                    MorphTargetInfo& morph_info = morph_targets_[prim_info->morph_target_];
                                    const fgltf::Accessor& acc = gltf->accessors[attrib.accessorIndex];
                                    fgltf::iterateAccessorWithIndex<fgltf::math::fvec3>(
                                        *gltf, acc, //
                                        [&](const fgltf::math::fvec3& position, size_t iter)
                                        {
                                            size_t pos_idx = morph_info.first_position_ / 3            //
                                                             + morph_info.position_morph_count_ * iter //
                                                             + target_idx;
                                            glms::assign_value(target_positions_[pos_idx], position);
                                        });
                                    break;
                                }
                            }
                            target_idx++;
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        size_t target_idx = 0;
                        for (const auto& target : prim.targets)
                        {
                            for (const auto& attrib : target)
                            {
                                if (attrib.name == "NORMAL")
                                {
                                    MorphTargetInfo& morph_info = morph_targets_[prim_info->morph_target_];
                                    const fgltf::Accessor& acc = gltf->accessors[attrib.accessorIndex];
                                    fgltf::iterateAccessorWithIndex<fgltf::math::fvec3>(
                                        *gltf, acc, //
                                        [&](const fgltf::math::fvec3& normal, size_t iter)
                                        {
                                            size_t normal_idx = morph_info.first_normal_ / 3            //
                                                                + morph_info.normal_morph_count_ * iter //
                                                                + target_idx;
                                            glms::assign_value(target_normals_[normal_idx], normal);
                                        });
                                    break;
                                }
                            }
                            target_idx++;
                        }
                        prim_info++;
                    }
                }
            }));

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_prim]()
            {
                auto prim_info = primitives_.begin() + first_prim;
                for (TSMeshIdx m_g(0); m_g < gltf->meshes.size(); m_g++)
                {
                    for (const fgltf::Primitive& prim : gltf->meshes[m_g].primitives)
                    {
                        size_t target_idx = 0;
                        for (const auto& target : prim.targets)
                        {
                            for (const auto& attrib : target)
                            {
                                if (attrib.name == "TANGENT")
                                {
                                    MorphTargetInfo& morph_info = morph_targets_[prim_info->morph_target_];
                                    const fgltf::Accessor& acc = gltf->accessors[attrib.accessorIndex];
                                    fgltf::iterateAccessorWithIndex<fgltf::math::fvec4>(
                                        *gltf, acc, //
                                        [&](const fgltf::math::fvec4& tangent, size_t iter)
                                        {
                                            size_t tangent_idx = morph_info.first_tangent_ / 4            //
                                                                 + morph_info.tangent_morph_count_ * iter //
                                                                 + target_idx;
                                            glms::assign_value(target_tangents_[tangent_idx], tangent);
                                        });
                                    break;
                                }
                            }
                            target_idx++;
                        }
                        prim_info++;
                    }
                }
            }));
    }

    // load textures
    std::mutex queue_lock;
    for (size_t g = 0; g < gltf_.size(); g++)
    {
        fgltf::Asset* gltf = &gltf_[g].get();
        TSTexIdx first_tex = first_tex_[g];
        TSSamplerIdx first_sampler = first_sampler_[g];

        futs.emplace_back(th_pool.submit_task(
            [this, gltf, first_tex, first_sampler, &queue_lock]()
            {
                vk::CommandPoolCreateInfo tex_cmd_pool_info{vk::CommandPoolCreateFlagBits::eResetCommandBuffer};
                vk::CommandPool tex_cmd_pool = device().createCommandPool(tex_cmd_pool_info);
                vk::CommandBufferAllocateInfo cmd_alloc_info{tex_cmd_pool, {}, 1};
                vk::CommandBuffer tex_cmd = device().allocateCommandBuffers(cmd_alloc_info)[0];
                Fence upload_completed;
                device().resetFences(upload_completed);

                auto extract_mipmaped = [](fgltf::Filter filter)
                {
                    switch (filter)
                    {
                        case fgltf::Filter::NearestMipMapNearest:
                        case fgltf::Filter::LinearMipMapNearest:
                        case fgltf::Filter::NearestMipMapLinear:
                        case fgltf::Filter::LinearMipMapLinear:
                            return true;
                        default:
                            return false;
                    }
                };

                for (TSSamplerIdx s_g(0); s_g < gltf->samplers.size(); s_g++)
                {
                    auto extract_filter = [](fgltf::Filter filter)
                    {
                        switch (filter)
                        {
                            case fgltf::Filter::Nearest:
                            case fgltf::Filter::NearestMipMapNearest:
                            case fgltf::Filter::NearestMipMapLinear:
                                return vk::Filter::eNearest;

                            case fgltf::Filter::Linear:
                            case fgltf::Filter::LinearMipMapNearest:
                            case fgltf::Filter::LinearMipMapLinear:
                            default:
                                return vk::Filter::eLinear;
                        }
                    };

                    auto extract_mipmap_mode = [](fgltf::Filter filter)
                    {
                        switch (filter)
                        {
                            case fgltf::Filter::NearestMipMapNearest:
                            case fgltf::Filter::LinearMipMapNearest:
                                return vk::SamplerMipmapMode::eNearest;

                            case fgltf::Filter::NearestMipMapLinear:
                            case fgltf::Filter::LinearMipMapLinear:
                            default:
                                return vk::SamplerMipmapMode::eLinear;
                        }
                    };

                    auto extract_wrapping = [](fgltf::Wrap wrap)
                    {
                        switch (wrap)
                        {
                            case fgltf::Wrap::ClampToEdge:
                                return vk::SamplerAddressMode::eClampToEdge;
                            case fgltf::Wrap::MirroredRepeat:
                                return vk::SamplerAddressMode::eMirroredRepeat;
                            default:
                            case fgltf::Wrap::Repeat:
                                return vk::SamplerAddressMode::eRepeat;
                        }
                    };

                    const fgltf::Sampler& sampler = gltf->samplers[s_g];
                    TSSamplerIdx s(first_sampler + s_g);
                    vk::SamplerCreateInfo sampler_info{};
                    sampler_info.anisotropyEnable = true;
                    sampler_info.maxAnisotropy = 4;
                    sampler_info.addressModeU = extract_wrapping(sampler.wrapS);
                    sampler_info.addressModeV = extract_wrapping(sampler.wrapT);
                    sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
                    sampler_info.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fgltf::Filter::Linear));
                    sampler_info.minFilter = extract_filter(sampler.minFilter.value_or(fgltf::Filter::Linear));
                    sampler_info.magFilter = extract_filter(sampler.magFilter.value_or(fgltf::Filter::Linear));
                    sampler_info.maxLod = extract_mipmaped(sampler.minFilter.value_or(fgltf::Filter::Linear)) //
                                              ? VK_LOD_CLAMP_NONE
                                              : 0;
                    samplers_[s] = device().createSampler(sampler_info);
                }

                for (TSTexIdx t_g(0); t_g < gltf->textures.size(); t_g++)
                {
                    const fgltf::Image& img_g = gltf->images[gltf->textures[t_g].imageIndex.value()];
                    const fgltf::BufferView& img_bufer_view =
                        gltf->bufferViews[std::get<1>(img_g.data).bufferViewIndex];
                    const fgltf::Buffer& img_bufer = gltf->buffers[img_bufer_view.bufferIndex];

                    int w = 0, h = 0, c = 0;
                    stbi_uc* pixels = stbi_load_from_memory(
                        (const unsigned char*)std::get<3>(img_bufer.data).bytes.data() + img_bufer_view.byteOffset,
                        img_bufer_view.byteLength, //
                        &w, &h, &c,                //
                        STBI_rgb_alpha);
                    size_t img_size = w * h * 4;

                    bool mip_mapping = extract_mipmaped(gltf->samplers[gltf->textures[t_g].samplerIndex.value_or(0)] //
                                                            .minFilter.value_or(fgltf::Filter::Linear));
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

                    Buffer<BufferBase::EmptyExtraInfo, seq_write, host_coherent, vertex> staging(img_size, SRC);
                    memcpy(staging.map_memory(), pixels, img_size);
                    staging.unmap_memory();
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
                    queue_lock.lock();
                    queues(GRAPHICS).submit(submit, upload_completed);
                    queue_lock.unlock();

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

    // copy data to gpu
    while (sizeof_arr(idxs_) % 16)
    {
        idxs_.push_back(-1);
    }
    size_t vtx_positions_padding = sizeof_arr(vtx_positions_) % 16 //
                                       ? 16 - sizeof_arr(vtx_positions_) % 16
                                       : (vtx_positions_.empty() ? 16 : 0);
    size_t vtx_normals_padding = sizeof_arr(vtx_normals_) % 16 //
                                     ? 16 - sizeof_arr(vtx_normals_) % 16
                                     : (vtx_normals_.empty() ? 16 : 0);
    size_t vtx_tangents_padding = sizeof_arr(vtx_tangents_) % 16 //
                                      ? 16 - sizeof_arr(vtx_tangents_) % 16
                                      : (vtx_tangents_.empty() ? 16 : 0);
    size_t vtx_texcoords_padding = sizeof_arr(vtx_texcoords_) % 16 //
                                       ? 16 - sizeof_arr(vtx_texcoords_) % 16
                                       : (vtx_texcoords_.empty() ? 16 : 0);
    size_t vtx_colors_padding = sizeof_arr(vtx_colors_) % 16 //
                                    ? 16 - sizeof_arr(vtx_colors_) % 16
                                    : (vtx_colors_.empty() ? 16 : 0);
    size_t vtx_joints_padding = sizeof_arr(vtx_joints_) % 16 //
                                    ? 16 - sizeof_arr(vtx_joints_) % 16
                                    : (vtx_joints_.empty() ? 16 : 0);
    size_t vtx_weights_padding = sizeof_arr(vtx_weights_) % 16 //
                                     ? 16 - sizeof_arr(vtx_weights_) % 16
                                     : (vtx_weights_.empty() ? 16 : 0);
    size_t target_positions_padding = sizeof_arr(target_positions_) % 16 //
                                          ? 16 - sizeof_arr(target_positions_) % 16
                                          : (target_positions_.empty() ? 16 : 0);
    size_t target_normals_padding = sizeof_arr(target_normals_) % 16 //
                                        ? 16 - sizeof_arr(target_normals_) % 16
                                        : (target_normals_.empty() ? 16 : 0);
    size_t target_tangents_padding = sizeof_arr(target_tangents_) % 16 //
                                         ? 16 - sizeof_arr(target_tangents_) % 16
                                         : (target_tangents_.empty() ? 16 : 0);
    size_t draw_calls_padding = sizeof_arr(draw_calls_) % 16 //
                                    ? 16 - sizeof_arr(draw_calls_) % 16
                                    : (draw_calls_.empty() ? 16 : 0);
    size_t meshes_padding = sizeof_arr(meshes_) % 16 //
                                ? 16 - sizeof_arr(meshes_) % 16
                                : (meshes_.empty() ? 16 : 0);
    size_t morph_targets_padding = sizeof_arr(morph_targets_) % 16 //
                                       ? 16 - sizeof_arr(morph_targets_) % 16
                                       : (morph_targets_.empty() ? 16 : 0);
    size_t primitives_padding = sizeof_arr(primitives_) % 16 //
                                    ? 16 - sizeof_arr(primitives_) % 16
                                    : (primitives_.empty() ? 16 : 0);
    size_t materials_padding = sizeof_arr(materials_) % 16 //
                                   ? 16 - sizeof_arr(materials_) % 16
                                   : (materials_.empty() ? 16 : 0);

    make_unique2(buffer_,
                 sizeof_arr(idxs_)                                              //
                     + sizeof_arr(vtx_positions_) + vtx_positions_padding       //
                     + sizeof_arr(vtx_normals_) + vtx_normals_padding           //
                     + sizeof_arr(vtx_tangents_) + vtx_tangents_padding         //
                     + sizeof_arr(vtx_texcoords_) + vtx_texcoords_padding       //
                     + sizeof_arr(vtx_colors_) + vtx_colors_padding             //
                     + sizeof_arr(vtx_joints_) + vtx_joints_padding             //
                     + sizeof_arr(vtx_weights_) + vtx_weights_padding           //
                     + sizeof_arr(target_positions_) + target_positions_padding //
                     + sizeof_arr(target_normals_) + target_normals_padding     //
                     + sizeof_arr(target_tangents_) + target_tangents_padding   //
                     + sizeof_arr(draw_calls_) + draw_calls_padding             //
                     + sizeof_arr(meshes_) + meshes_padding                     //
                     + sizeof_arr(morph_targets_) + morph_targets_padding       //
                     + sizeof_arr(primitives_) + primitives_padding             //
                     + sizeof_arr(materials_) + materials_padding,
                 DST);
    buffer_->vtx_positions_ = sizeof_arr(idxs_);
    buffer_->vtx_normals_ = buffer_->vtx_positions_ + sizeof_arr(vtx_positions_) + vtx_positions_padding;
    buffer_->vtx_tangents_ = buffer_->vtx_normals_ + sizeof_arr(vtx_normals_) + vtx_normals_padding;
    buffer_->vtx_texcoords_ = buffer_->vtx_tangents_ + sizeof_arr(vtx_tangents_) + vtx_tangents_padding;
    buffer_->vtx_colors_ = buffer_->vtx_texcoords_ + sizeof_arr(vtx_texcoords_) + vtx_texcoords_padding;
    buffer_->vtx_joints_ = buffer_->vtx_colors_ + sizeof_arr(vtx_colors_) + vtx_colors_padding;
    buffer_->vtx_weights_ = buffer_->vtx_joints_ + sizeof_arr(vtx_joints_) + vtx_joints_padding;
    buffer_->target_positions_ = buffer_->vtx_weights_ + sizeof_arr(vtx_weights_) + vtx_weights_padding;
    buffer_->target_normals_ = buffer_->target_positions_ + sizeof_arr(target_positions_) + target_positions_padding;
    buffer_->target_tangents_ = buffer_->target_normals_ + sizeof_arr(target_normals_) + target_normals_padding;
    buffer_->draw_calls_ = buffer_->target_tangents_ + sizeof_arr(target_tangents_) + target_tangents_padding;
    buffer_->meshes_ = buffer_->draw_calls_ + sizeof_arr(draw_calls_) + draw_calls_padding;
    buffer_->morph_targets_ = buffer_->meshes_ + sizeof_arr(meshes_) + meshes_padding;
    buffer_->primitives_ = buffer_->morph_targets_ + sizeof_arr(morph_targets_) + morph_targets_padding;
    buffer_->materials_ = buffer_->primitives_ + sizeof_arr(primitives_) + primitives_padding;

    Buffer<BufferBase::EmptyExtraInfo, vertex, seq_write, host_cached> staging(buffer_->size(), SRC);
    staging.map_memory();
    memcpy(staging.mapping() + buffer_->idx_, idxs_.data(), sizeof_arr(idxs_));
    memcpy(staging.mapping() + buffer_->vtx_positions_, vtx_positions_.data(), sizeof_arr(vtx_positions_));
    memcpy(staging.mapping() + buffer_->vtx_normals_, vtx_normals_.data(), sizeof_arr(vtx_normals_));
    memcpy(staging.mapping() + buffer_->vtx_tangents_, vtx_tangents_.data(), sizeof_arr(vtx_tangents_));
    memcpy(staging.mapping() + buffer_->vtx_texcoords_, vtx_texcoords_.data(), sizeof_arr(vtx_texcoords_));
    memcpy(staging.mapping() + buffer_->vtx_colors_, vtx_colors_.data(), sizeof_arr(vtx_colors_));
    memcpy(staging.mapping() + buffer_->vtx_joints_, vtx_joints_.data(), sizeof_arr(vtx_joints_));
    memcpy(staging.mapping() + buffer_->vtx_weights_, vtx_weights_.data(), sizeof_arr(vtx_weights_));
    memcpy(staging.mapping() + buffer_->target_positions_, target_positions_.data(), sizeof_arr(target_positions_));
    memcpy(staging.mapping() + buffer_->target_normals_, target_normals_.data(), sizeof_arr(target_normals_));
    memcpy(staging.mapping() + buffer_->target_tangents_, target_tangents_.data(), sizeof_arr(target_tangents_));
    memcpy(staging.mapping() + buffer_->draw_calls_, draw_calls_.data(), sizeof_arr(draw_calls_));
    memcpy(staging.mapping() + buffer_->meshes_, meshes_.data(), sizeof_arr(meshes_));
    memcpy(staging.mapping() + buffer_->morph_targets_, morph_targets_.data(), sizeof_arr(morph_targets_));
    memcpy(staging.mapping() + buffer_->primitives_, primitives_.data(), sizeof_arr(primitives_));
    memcpy(staging.mapping() + buffer_->materials_, materials_.data(), sizeof_arr(materials_));
    staging.flush_cache();
    staging.unmap_memory();

    vk::CommandBuffer cmd = one_time_submit_cmd();
    begin_cmd(cmd);
    cmd.copyBuffer(staging, *buffer_, {{0, 0, buffer_->size()}});
    cmd.end();
    submit_one_time_cmd(cmd);

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    std::array<vk::DescriptorSetLayoutBinding, 16> bindings = {};
    for (uint32_t b = 0; b < bindings.size(); b++)
    {
        bindings[b].binding = b;
        bindings[b].descriptorCount = 1;
        bindings[b].descriptorType = vk::DescriptorType::eStorageBuffer;
        bindings[b].stageFlags = vk::ShaderStageFlagBits::eAll;
    }
    bindings[15].descriptorCount = tex_infos_.size();
    bindings[15].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    bindings[15].stageFlags = vk::ShaderStageFlagBits::eFragment;
    layout_info.setBindings(bindings);
    set_layout_ = device().createDescriptorSetLayout(layout_info);

    des_sizes_[0].type = vk::DescriptorType::eCombinedImageSampler;
    des_sizes_[0].descriptorCount = tex_infos_.size();
    des_sizes_[1].type = vk::DescriptorType::eStorageBuffer;
    des_sizes_[1].descriptorCount = 15;
}

void fi::ResDetails::bind_res(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t des_set)
{
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, des_set, des_set_, {});
    cmd.bindIndexBuffer(*buffer_, buffer_->idx_, vk::IndexType::eUint32);
}

bool fi::ResDetails::locked() const
{
    return locked_;
}

void fi::ResDetails::draw(vk::CommandBuffer cmd)
{
    cmd.drawIndexedIndirect(*buffer_,             //
                            buffer_->draw_calls_, //
                            draw_call_count_,     //
                            sizeof(vk::DrawIndexedIndirectCommand));
}

void fi::ResDetails::allocate_descriptor(vk::DescriptorPool des_pool)
{
    if (!locked_)
    {
        return;
    }

    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool = des_pool;
    alloc_info.setSetLayouts(set_layout_);
    des_set_ = device().allocateDescriptorSets(alloc_info)[0];

    vk::WriteDescriptorSet write_textures{};
    write_textures.dstSet = des_set_;
    write_textures.dstBinding = 15;
    write_textures.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write_textures.setImageInfo(tex_infos_);

    std::array<vk::DescriptorBufferInfo, 15> ssbo_infos{};
    for (auto& ssbo : ssbo_infos)
    {
        ssbo.buffer = *buffer_;
    }
    ssbo_infos[0].offset = buffer_->vtx_positions_;
    ssbo_infos[1].offset = buffer_->vtx_normals_;
    ssbo_infos[2].offset = buffer_->vtx_tangents_;
    ssbo_infos[3].offset = buffer_->vtx_texcoords_;
    ssbo_infos[4].offset = buffer_->vtx_colors_;
    ssbo_infos[5].offset = buffer_->vtx_joints_;
    ssbo_infos[6].offset = buffer_->vtx_weights_;
    ssbo_infos[7].offset = buffer_->target_positions_;
    ssbo_infos[8].offset = buffer_->target_normals_;
    ssbo_infos[9].offset = buffer_->target_tangents_;
    ssbo_infos[10].offset = buffer_->draw_calls_;
    ssbo_infos[11].offset = buffer_->meshes_;
    ssbo_infos[12].offset = buffer_->morph_targets_;
    ssbo_infos[13].offset = buffer_->primitives_;
    ssbo_infos[14].offset = buffer_->materials_;

    ssbo_infos[0].range = buffer_->vtx_normals_ - buffer_->vtx_positions_;
    ssbo_infos[1].range = buffer_->vtx_tangents_ - buffer_->vtx_normals_;
    ssbo_infos[2].range = buffer_->vtx_texcoords_ - buffer_->vtx_tangents_;
    ssbo_infos[3].range = buffer_->vtx_colors_ - buffer_->vtx_texcoords_;
    ssbo_infos[4].range = buffer_->vtx_joints_ - buffer_->vtx_colors_;
    ssbo_infos[5].range = buffer_->vtx_weights_ - buffer_->vtx_joints_;
    ssbo_infos[6].range = buffer_->target_positions_ - buffer_->vtx_weights_;
    ssbo_infos[7].range = buffer_->target_normals_ - buffer_->target_positions_;
    ssbo_infos[8].range = buffer_->target_tangents_ - buffer_->target_normals_;
    ssbo_infos[9].range = buffer_->draw_calls_ - buffer_->target_tangents_;
    ssbo_infos[10].range = buffer_->meshes_ - buffer_->draw_calls_;
    ssbo_infos[11].range = buffer_->morph_targets_ - buffer_->meshes_;
    ssbo_infos[12].range = buffer_->primitives_ - buffer_->morph_targets_;
    ssbo_infos[13].range = buffer_->materials_ - buffer_->primitives_;
    ssbo_infos[14].range = buffer_->size() - buffer_->materials_;

    vk::WriteDescriptorSet write_ssbos{};
    write_ssbos.dstSet = des_set_;
    write_ssbos.dstBinding = 0;
    write_ssbos.descriptorType = vk::DescriptorType::eStorageBuffer;
    write_ssbos.setBufferInfo(ssbo_infos);
    device().updateDescriptorSets({write_textures, write_ssbos}, {});

    free_stl_container(idxs_);
    free_stl_container(vtx_positions_);
    free_stl_container(vtx_normals_);
    free_stl_container(vtx_tangents_);
    free_stl_container(vtx_texcoords_);
    free_stl_container(vtx_colors_);
    free_stl_container(vtx_joints_);
    free_stl_container(vtx_weights_);
    free_stl_container(target_positions_);
    free_stl_container(target_normals_);
    free_stl_container(target_tangents_);

    free_stl_container(meshes_);
    free_stl_container(morph_targets_);
    free_stl_container(primitives_);
    free_stl_container(materials_);
    free_stl_container(tex_infos_);

    draw_call_count_ = draw_calls_.size();
    free_stl_container(draw_calls_);
    free_stl_container(gltf_);
}