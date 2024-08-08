#ifndef GRAPHICS2_RES_UPLOADER_HPP
#define GRAPHICS2_RES_UPLOADER_HPP

#include "graphics.hpp"
#include "buffer.hpp"
#include <set>
#include <queue>

namespace fi
{
    struct ResVtx
    {
        glm::vec3 position_ = {0, 0, 0};
        glm::vec3 normal_ = {0, 0, 0};
        glm::vec4 tangent_ = {0, 0, 0, 1};
        glm::vec2 tex_coord_ = {0, 0};
        glm::vec4 color_ = {1, 1, 1, 1};
        glm::uvec4 joint_ = {-1, -1, -1, -1};
        glm::vec4 weight_ = {0, 0, 0, 0};
    };

    struct alignas(16) ResMaterial
    {
        glm::vec4 color_factor_ = {1, 1, 1, 1};
        glm::vec4 emissive_factor_ = {0, 0, 0, 1};    // [3] = emissive strength
        glm::vec4 sheen_color_factor_ = {0, 0, 0, 0}; // [3] = sheen roughtness factor
        glm::vec4 spec_factor_ = {1, 1, 1, 1};        // [3] = place holder

        float alpha_cutoff_ = 0;
        float metalic_ = 1.0f;
        float roughtness_ = 1.0f;
        uint32_t color_texture_idx_ = 0;
        uint32_t metalic_roughtness_ = ~0;

        uint32_t normal_map_idx_ = ~0;
        uint32_t emissive_map_idx_ = ~0;
        uint32_t occlusion_map_idx_ = ~0;

        float anistropy_rotation_ = 0;
        float anistropy_strength_ = 0;
        uint32_t anistropy_map_idx_ = ~0;

        uint32_t spec_map_idx_ = ~0;
        uint32_t spec_color_map_idx_ = ~0;

        uint32_t sheen_color_map_idx_ = ~0;
        uint32_t sheen_roughtness_map_idx_ = ~0;
    };

    struct ResMesh
    {
        std::string name_{};
        uint32_t primitive_count_ = 0;
        size_t primitive_idx_ = 0;
    };

    struct ResDetails : private GraphicsObject
    {
      private:
        gltf::Model model_{};
        void add_texture(unsigned char* pixels, uint32_t w, uint32_t h);

      public:
        inline const static char* EXTENSIONS[] = {"KHR_materials_emissive_strength", //
                                                  "KHR_materials_transmission",      //
                                                  "KHR_materials_ior",               //
                                                  "KHR_materials_volume",            //
                                                  "KHR_materials_sheen",             //
                                                  "KHR_materials_specular"};

        // storage
        std::vector<vk::Image> textures_{};
        std::vector<vk::ImageView> texture_views_{};
        std::vector<vma::Allocation> texture_allocs_{};
        std::vector<vk::DescriptorImageInfo> tex_infos_{};

        std::vector<vk::Sampler> samplers_{};
        std::vector<ResMaterial> materials_{}; // indexed by material_idxs_[prim]

        std::array<vk::DescriptorPoolSize, 2> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{}; // bind to fragment shader

        struct DeviceBufferOffsets
        {
            vk::DeviceSize vtx_buffer_ = 0;
            vk::DeviceSize idx_buffer_ = 0;
            vk::DeviceSize materials_ = 0;
            vk::DeviceSize material_idxs_ = 0;
            vk::DeviceSize draw_calls_ = 0;
        };

        size_t prim_count_ = 0;
        std::unique_ptr<Buffer<DeviceBufferOffsets, vertex, index, indirect, storage>> buffer_{};
        std::vector<uint32_t> material_idxs_{}; // indexed by prim
        std::vector<ResMesh> meshes_{};         // indexed by mesh

        ResDetails(const std::filesystem::path& path);
        ~ResDetails();

        void allocate_descriptor(vk::DescriptorPool des_pool);
        [[nodiscard]] const gltf::Model& model() const;
        void bind(vk::CommandBuffer cmd, uint32_t buffer_binding, vk::PipelineLayout pipeline_layout, uint32_t set);
        void draw(vk::CommandBuffer cmd);

        // instancing will need another buffer of glm::mat4s;
        static void set_pipeline_create_details(std::vector<vk::VertexInputBindingDescription>& binding_des,
                                                std::vector<vk::VertexInputAttributeDescription>& attrib_des,
                                                uint32_t buffer_binding = 0, bool instancing = false);
    };

    struct ResSkin
    {
        std::string name_ = "";
        size_t joint_idx_ = 0;
        size_t joint_count_ = 0;
    };

    struct ResSkinDetails : private GraphicsObject
    {

        struct SkinOffsets
        {
            vk::DeviceSize joint_idx_ = 0;
            vk::DeviceSize inv_matrices_ = 0;
            vk::DeviceSize joints_ = 0;
        };

        std::vector<ResSkin> skins_{}; // index by skin

        std::array<vk::DescriptorPoolSize, 1> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{}; // bind to vertex shader
        std::unique_ptr<Buffer<SkinOffsets, storage>> buffer_{};
        std::vector<uint32_t> joint_idx_{};     // indexed by prim
        std::vector<glm::mat4> inv_matrices_{}; // indexed by joint_idx_[prim] + shader_attribute
        std::vector<uint32_t> joints_{};        // indexed by joint_idx_[prim] + shader_attribute, store as node_idx

        ResSkinDetails(const ResDetails& res_details);
        ~ResSkinDetails();

        [[nodiscard]] bool empty() const { return skins_.empty(); }
        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set);
    };

    struct ResMorph
    {
    };

    struct ResMorphDetails
    {
    };
}; // namespace fi

#endif // GRAPHICS2_RES_UPLOADER_HPP
