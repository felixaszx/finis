#ifndef GRAPHICS2_RES_UPLOADER_HPP
#define GRAPHICS2_RES_UPLOADER_HPP

#include "graphics.hpp"
#include "buffer.hpp"

namespace fi
{
    struct alignas(16) Material
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

    struct ResDetails : private GraphicsObject
    {
      private:
        gltf::Model model_{};
        void load_mip_maps();

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
        std::vector<vk::Sampler> samplers_{};
        std::vector<Material> materials_{};

        std::vector<glm::vec3> positions_{};
        std::vector<glm::vec3> normals_{};
        std::vector<glm::vec4> tangents_{};
        std::vector<glm::vec2> tex_coords_{};
        std::vector<glm::vec4> colors_{};
        std::vector<glm::u16vec4> joints_{};
        std::vector<glm::vec4> weights_{};

        struct DeviceBufferOffsets
        {
            vk::DeviceSize positions_ = 0;
            vk::DeviceSize normals_ = 0;
            vk::DeviceSize tangents_ = 0;
            vk::DeviceSize tex_coords_ = 0;
            vk::DeviceSize colors_ = 0;
            vk::DeviceSize joints_ = 0;
            vk::DeviceSize weights_ = 0;
            vk::DeviceSize indices_ = 0;
        };

        // accessors
        std::unique_ptr<Buffer<DeviceBufferOffsets, vertex, index, storage>> device_buffer_{};
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, indirect, storage, seq_write>> host_buffer_{};
        std::vector<uint32_t> material_idxs_{};

        ResDetails(const std::filesystem::path& path);
        ~ResDetails();

        void add_texture(unsigned char* pixels, uint32_t w, uint32_t h);
    };

}; // namespace fi

#endif // GRAPHICS2_RES_UPLOADER_HPP
