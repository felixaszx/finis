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
        size_t old_target_positions_count = 0; // tbd
        size_t old_target_normals_count_ = 0;  // tbd
        size_t old_target_tangents_count_ = 0; // tbd

        // indexed by size_t in scalar offset
        std::vector<uint32_t> idxs_{};
        std::vector<glm::vec3> vtx_positions_{};
        std::vector<glm::vec3> vtx_normals_{};
        std::vector<glm::vec4> vtx_tangents_{};
        std::vector<glm::vec2> vtx_texcoords_{};
        std::vector<glm::vec4> vtx_colors_{};
        std::vector<glm::uvec4> vtx_joints_{};
        std::vector<glm::vec4> vtx_weights_{};
        std::vector<glm::vec3> target_positions_{};                // tbd
        std::vector<glm::vec3> target_normals_{};                  // tbd
        std::vector<glm::vec4> target_tangents_{};                 // tbd
        std::vector<vk::DrawIndexedIndirectCommand> draw_calls_{}; // indexed by PrimIdx

        // hlper infos
        std::vector<PrimIdx> first_prim_{};
        std::vector<gltf::Expected<gltf::GltfDataBuffer>> gltf_file_{};
        std::vector<gltf::Expected<gltf::Asset>> gltf_{};
        std::vector<TSTexIdx> first_tex_{};
        std::vector<TSSamplerIdx> first_sampler_{};
        std::vector<TSMaterialIdx> first_material_{};
        std::vector<TSMeshIdx> first_mesh_{};
        std::vector<TSMorphTargetIdx> first_morph_target_{};

        // storage
        std::vector<vk::Image> tex_imgs_{};
        std::vector<vk::ImageView> tex_views_{};
        std::vector<vma::Allocation> tex_allocs_{};
        std::vector<vk::DescriptorImageInfo> tex_infos_{};
        std::vector<vk::Sampler> samplers_{};

      public:
        // gpu storage, set by other calls
        std::vector<MeshInfo> meshes_{};               // indexed by TSMeshIdx
        std::vector<MorphTargetInfo> morph_targets_{}; // indexed by TSMorphTarget
        std::vector<PrimInfo> primitives_{};           // indexed by PrimIdx
        std::vector<MaterialInfo> materials_{};        // indexed by TSMaterialIdx

        ~ResDetails();

        void add_gltf_file(const std::filesystem::path& path);
        void lock_and_load();
    };
}; // namespace fi

#endif // GRAPHICS_RES_LOADER_HPP
