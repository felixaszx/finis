#include "graphics/res_loader.hpp"
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
    for (size_t g = 0; g < gltf_.size(); g++)
    {
        gltf::Asset* gltf = &gltf_[g].get();
        TSTexIdx first_tex = first_tex_[g];
        TSSamplerIdx first_sampler = first_sampler_[g];
        TSMaterialIdx first_material = first_material_[g];
        TSMeshIdx first_mesh = first_mesh_[g];
        PrimIdx first_prim = first_prim_[g];

        std::future<void> idx_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_pos_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_normal_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_tangent_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_texcoord_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_color_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_joints_fut = th_pool_.submit_task(
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
            });

        std::future<void> vtx_weights_fut = th_pool_.submit_task(
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
            });
    }
    th_pool_.wait();
}
