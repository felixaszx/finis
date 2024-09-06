#include "graphics/prim_res.hpp"
#include "resources/gltf_file.hpp"
#include "resources/gltf_structure.hpp"

using namespace fi;
using namespace fi::util::literals;
class loaded_res : public gfx::prim_res
{
  private:
    std::unique_ptr<gfx::primitives> primitives_{};
    std::unique_ptr<gfx::prim_structure> prim_struct_{};
    std::unique_ptr<gfx::prim_skins> prim_skins_{};
    std::unique_ptr<gfx::tex_arr> tex_arr_{};

  public:
    loaded_res()
    {
        thp::task_thread_pool thread_pool;
        vk::CommandPoolCreateInfo pool_info{};
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = queue_indices(gfx::context::GRAPHICS);
        vk::CommandPool cmd_pool = device().createCommandPool(pool_info);

        std::vector<std::future<void>> futs;
        res::gltf_file sparta("res/models/sparta.glb", &futs, &thread_pool);
        std::for_each(futs.begin(), futs.end(), [](std::future<void>& fut) { fut.wait(); });
        res::gltf_structure sparta_struct(sparta);
        res::gltf_skins sparta_skin(sparta);

        util::make_unique2(prim_skins_, sparta.meshes_.size());
        for (uint32_t s = 0; s < sparta_skin.skins_.size(); s++)
        {
            prim_skins_->add_skin(sparta_skin.skins_[s], sparta_skin.inv_binds_[s]);
        }

        util::make_unique2(tex_arr_);
        for (auto& sampler : sparta.samplers_)
        {
            tex_arr_->add_sampler(sampler);
        }
        for (auto& tex : sparta.textures_)
        {
            tex_arr_->add_tex(cmd_pool, tex.sampler_idx_, tex.data_, tex.get_extent(), tex.get_levels());
        }

        util::make_unique2(prim_struct_, sparta.prim_count());

        util::make_unique2(primitives_, 20_mb, 2000);
        primitives_->generate_staging_buffer(1_mb);
        for (uint32_t m = 0; m < sparta.meshes_.size(); m++)
        {
            std::vector<uint32_t> prim_idxs(sparta.meshes_[m].prims_.size());
            for (uint32_t p = 0; p < sparta.meshes_[m].prims_.size(); p++)
            {
                prim_idxs[p] = primitives_->prims_.count_;
                res::gltf_prim& prim = sparta.meshes_[m].prims_[p];
                primitives_->add_primitives({sparta.meshes_[m].draw_calls_[p]});
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::POSITON, prim.positions_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::NORMAL, prim.normals_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::TANGENT, prim.tangents_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::TEXCOORD, prim.texcoords_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::COLOR, prim.colors_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::JOINTS, prim.joints_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::WEIGHTS, prim.weights_);
                primitives_->add_attribute_data(cmd_pool, gfx::prim_info::INDEX, prim.idxs_);
            }

            uint32_t node_idx = sparta_struct.mesh_nodes_[m];
            prim_struct_->add_mesh(prim_idxs,                            //
                                   sparta_struct.seq_mapping_[node_idx], //
                                   node_idx,                             //
                                   sparta_struct.nodes_[node_idx]);

            uint32_t skin_idx = sparta_skin.mesh_skin_idxs_[m];
            if (skin_idx != -1)
            {
                prim_skins_->set_skin(m, skin_idx);
            }
        }
        prim_struct_->load_data();
        prim_skins_->load_data(cmd_pool);
        primitives_->reload_draw_calls(cmd_pool);
        primitives_->free_staging_buffer();

        thread_pool.wait_for_tasks();
        device().destroyCommandPool(cmd_pool);
    }

    ~loaded_res() {}

    gfx::primitives* get_primitives() override { return primitives_.get(); }
    gfx::prim_structure* get_prim_structure() override { return prim_struct_.get(); }
    gfx::prim_skins* get_prim_skin() override { return prim_skins_.get(); }
    gfx::tex_arr* get_tex_arr() override { return tex_arr_.get(); }
};

EXPORT_EXTENSION(loaded_res);