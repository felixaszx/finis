#ifndef GRAPHICS_RES_LOADER_HPP
#define GRAPHICS_RES_LOADER_HPP

#include <stb/stb_image.h>

#include "res_data.hpp"
#include "graphics.hpp"
#include "graphics/buffer.hpp"

#include "bs_th_pool/BS_thread_pool.hpp"
#include "bs_th_pool/BS_thread_pool_utils.hpp"

namespace fi
{
    namespace bs = BS;
    class ResDetails : private GraphicsObject
    {
      private:
        bool locked_ = false;
        // threading helpers
        bs::thread_pool th_pool_;
        // indexed by size_t in vector offsets
        size_t old_vtx_count_ = 0;
        size_t old_idx_count_ = 0;
        size_t old_normals_count_ = 0;
        size_t old_tangents_count_ = 0;
        size_t old_texcoords_count_ = 0;
        size_t old_colors_count_ = 0;
        size_t old_joints_count_ = 0;
        size_t old_weights_count_ = 0;
        size_t old_target_positions_count = 0;
        size_t old_target_normals_count_ = 0;
        size_t old_target_tangents_count_ = 0;

        // indexed by size_t in scalar offset
        std::vector<uint32_t> idxs_{};
        std::vector<glm::vec3> vtx_positions_{};
        std::vector<glm::vec3> vtx_normals_{};
        std::vector<glm::vec4> vtx_tangents_{};
        std::vector<glm::vec2> vtx_texcoords_{};
        std::vector<glm::vec4> vtx_colors_{};
        std::vector<glm::uvec4> vtx_joints_{};
        std::vector<glm::vec4> vtx_weights_{};
        std::vector<glm::vec3> target_positions_{};                // tbd AOS style
        std::vector<glm::vec3> target_normals_{};                  // tbd AOS style
        std::vector<glm::vec4> target_tangents_{};                 // tbd AOS style
        std::vector<vk::DrawIndexedIndirectCommand> draw_calls_{}; // indexed by PrimIdx

        // helper infos
        std::vector<PrimIdx> first_prim_{};
        std::vector<gltf::Expected<gltf::Asset>> gltf_{};
        std::vector<TSTexIdx> first_tex_{};
        std::vector<TSSamplerIdx> first_sampler_{};
        std::vector<TSMaterialIdx> first_material_{};
        std::vector<TSMeshIdx> first_mesh_{};
        std::vector<TSMorphTargetIdx> first_morph_target_{};

        // storage
        uint32_t draw_call_count_ = 0;
        std::vector<vk::Image> tex_imgs_{};
        std::vector<vk::ImageView> tex_views_{};
        std::vector<vma::Allocation> tex_allocs_{};
        std::vector<vk::DescriptorImageInfo> tex_infos_{};
        std::vector<vk::Sampler> samplers_{};

        // defines
        struct BufferOffsets
        {
            vk::DeviceSize idx_ = 0;
            vk::DeviceSize vtx_positions_ = 0;    // binding 0
            vk::DeviceSize vtx_normals_ = 0;      // binding 1
            vk::DeviceSize vtx_tangents_ = 0;     // binding 2
            vk::DeviceSize vtx_texcoords_ = 0;    // binding 3
            vk::DeviceSize vtx_colors_ = 0;       // binding 4
            vk::DeviceSize vtx_joints_ = 0;       // binding 5
            vk::DeviceSize vtx_weights_ = 0;      // binding 6
            vk::DeviceSize target_positions_ = 0; // binding 7
            vk::DeviceSize target_normals_ = 0;   // binding 8
            vk::DeviceSize target_tangents_ = 0;  // binding 9

            vk::DeviceSize draw_calls_ = 0;    // binding 10
            vk::DeviceSize meshes_ = 0;        // binding 11
            vk::DeviceSize morph_targets_ = 0; // binding 12
            vk::DeviceSize primitives_ = 0;    // binding 13
            vk::DeviceSize materials_ = 0;     // binding 14
        };

      public:
        // descriptors
        std::array<vk::DescriptorPoolSize, 2> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{}; // binding order is the same as BufferOffsets, textures binding is 15

        // gpu storage, set by other calls
        std::vector<MeshInfo> meshes_{};               // indexed by TSMeshIdx
        std::vector<MorphTargetInfo> morph_targets_{}; // indexed by TSMorphTarget
        std::vector<PrimInfo> primitives_{};           // indexed by PrimIdx
        std::vector<MaterialInfo> materials_{};        // indexed by TSMaterialIdx
        std::unique_ptr<Buffer<BufferOffsets, storage, indirect, index>> buffer_;

        ~ResDetails();

        void add_gltf_file(const std::filesystem::path& path);
        void lock_and_load();
        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind_res(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t des_set);
        void draw(vk::CommandBuffer cmd);
    };
}; // namespace fi

#endif // GRAPHICS_RES_LOADER_HPP
