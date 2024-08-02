#ifndef GRAPHICS2_RES_UPLOADER_HPP
#define GRAPHICS2_RES_UPLOADER_HPP

#include "graphics.hpp"
#include "buffer.hpp"

namespace fi
{
    struct Vertex
    {
        glm::vec3 positon_{};
        glm::vec3 normal_{};
        glm::vec3 tangent_{};
        glm::vec2 tex_coord_{};
        glm::uvec4 bone_ids_{};
        glm::vec4 bone_weight_{};
    };

    struct alignas(16) Material
    {
        // pbr_data
        glm::vec4 color_factor_ = {1, 1, 1, 1}; // alignment
        float metalic_ = 1.0f;
        float roughtness_ = 1.0f;
        uint32_t color_texture_idx_ = 0;
        uint32_t metalic_roughtness_ = ~0; // alignment

        // emissive
        glm::vec4 combined_emissive_factor_ = {0, 0, 0, 1}; // [3] = emissive strength // alignment
        uint32_t emissive_map_idx_ = ~0;

        // occlusion  map
        uint32_t occlusion_map_idx_ = ~0;

        // alpha
        float alpha_value_ = 0;

        // normal
        float normal_scale_ = 0; // alignment
        uint32_t normal_map_idx_ = ~0;

        // anistropy
        float anistropy_rotation_ = 0;
        float anistropy_strength_ = 0;
        uint32_t anistropy_map_idx_ = ~0; // alginment

        // specular
        glm::vec4 combined_spec_factor_ = {1, 1, 1, 1}; // [3] = specular strength // alginment
        uint32_t spec_color_map_idx_ = ~0;
        uint32_t spec_map_idx_ = ~0;

        // transmission
        float transmission_factor_ = 0;
        uint32_t transmission_map_idx_ = ~0; // alignment

        // volume
        glm::vec4 combined_attenuation_ = {
            1, 1, 1,                                 //
            std::numeric_limits<float>::infinity()}; // [3] = attenuation distance // alignment
        float thickness_factor_ = 0;
        uint32_t thickness_map_idx_ = ~0;

        // sheen
        uint32_t sheen_color_map_idx_ = ~0;
        uint32_t sheen_roughtness_map_idx_ = ~0;               // alginment
        glm::vec4 combined_sheen_color_factor_ = {0, 0, 0, 0}; // [3] = sheen roughtness factor
    };

    struct ResDetails : private GraphicsObject
    {
      private:
        gltf::Model model_{};
        void load_mip_maps();

      public:
        // storage
        std::vector<vk::Image> textures_{};
        std::vector<vk::ImageView> texture_views_{};
        std::vector<vma::Allocation> texture_allocs_{};

        std::vector<vk::Sampler> samplers_{};
        std::vector<Material> materials_{};

        // acessors
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, vertex, index, storage>> device_buffer_{};
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, indirect, storage, seq_write>> host_buffer_{};
        std::vector<uint32_t> material_idxs_{};

        ResDetails(const std::filesystem::path& path);

        void add_texture(unsigned char* pixels, uint32_t w, uint32_t h);
    };

}; // namespace fi

#endif // GRAPHICS2_RES_UPLOADER_HPP
