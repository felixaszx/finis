#ifndef GRAPHICS_DIRECTIONAL_LIGHT_HPP
#define GRAPHICS_DIRECTIONAL_LIGHT_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "glms.hpp"

namespace fi
{
    class DirectionalLightData : private GraphicsObject
    {
      private:
        bool locked_ = false;
        vk::Image shadow_map_{};
        vk::ImageView map_view_{};
        vma::Allocation map_alloc_{};
        vk::Extent3D map_extent_ = {};

        vk::DescriptorSet des_set_{};
        vk::DescriptorSetLayout set_layout_{};

        vk::Pipeline pipeline_{};
        vk::PipelineLayout layout_{};

        vk::RenderingInfo rendering_{};
        vk::RenderingAttachmentInfo atchm_info_{};

      public:
        struct
        {
            glm::vec3 position_ = {0, 0, 0};
            glm::vec3 direction_ = {0, 0, 0};
            glm::vec3 color_ = {1, 1, 1};
        } light_data_;

        DirectionalLightData(vk::Extent2D resolution, uint32_t input_atchm_count, uint32_t output_count);
        ~DirectionalLightData();

        [[nodiscard]] vk::ImageView shadow_map_view() const;
        [[nodiscard]] vk::Format map_format() const;
        [[nodiscard]] vk::Extent3D map_extent() const;
        void lock_and_load(vk::DescriptorPool des_pool);
        void calculate_lighting(
            const std::function<void(vk::DescriptorSet input_atchms_set, uint32_t input_atchm_binding,
                                     uint32_t draw_vtx_cout)>& draw_func);
    };
}; // namespace fi

#endif // GRAPHICS_DIRECTIONAL_LIGHT_HPP