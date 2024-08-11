#ifndef GRAPHICS_RES_MESH_HPP
#define GRAPHICS_RES_MESH_HPP

#include "res_holder.hpp"

namespace fi
{
    class ResMeshDetails : private GraphicsObject
    {
      private:
        struct DeviceBufferOffsets
        {
            vk::DeviceSize vtx_ = 0;
            vk::DeviceSize idx_ = 0;
            vk::DeviceSize draw_calls_ = 0;

            vk::DeviceSize materials_ = 0;
            vk::DeviceSize prim_details_ = 0;
            vk::DeviceSize mesh_details_ = 0;
        };

      private:
        std::vector<MeshIdx> first_gltf_mesh_{};
        std::vector<uint32_t> gltf_mesh_count_{};

        std::vector<vk::Image> tex_imgs_{};
        std::vector<vk::ImageView> tex_views_{};
        std::vector<vma::Allocation> tex_allocs_{};
        std::vector<vk::DescriptorImageInfo> tex_infos_{};

        std::vector<vk::Sampler> samplers_{};
        std::vector<PrimMaterial> materials_{}; // indexed by PrimIdx

        std::vector<ResMesh> meshes_{};           // indexed by MeshIdx
        std::vector<PrimDetails> prim_details_{}; // indexed by PrimIdx
        std::vector<MeshDetails> mesh_details_{}; // indexed by MeshIdx

        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{}; // bind to fragment shader
        std::unique_ptr<Buffer<DeviceBufferOffsets, vertex, index, indirect, storage>> buffer_{};

        static std::array<vk::DescriptorPoolSize, 2> des_sizes_;

      public:
        ResMeshDetails(ResSceneDetails& target);
        ~ResMeshDetails();

        void allocate_gpu_res(vk::DescriptorPool des_pool);
        ResMesh& index_mesh(size_t gltf_idx, MeshIdx mesh_idx);
        MeshDetails& index_mesh_details(size_t gltf_idx, MeshIdx mesh_idx);
        void bind(vk::CommandBuffer cmd, uint32_t buffer_binding, vk::PipelineLayout pipeline_layout, uint32_t set);
        void draw(vk::CommandBuffer cmd);

        // instancing will need another buffer of glm::mat4s;
        static void set_pipeline_create_details(std::vector<vk::VertexInputBindingDescription>& binding_des,
                                                std::vector<vk::VertexInputAttributeDescription>& attrib_des,
                                                uint32_t buffer_binding = 0, bool instancing = false);
    };
}; // namespace fi

#endif // GRAPHICS_RES_MESH_HPP
