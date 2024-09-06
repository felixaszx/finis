#ifndef GRAPHICS_TEXTURES_HPP
#define GRAPHICS_TEXTURES_HPP

#include "graphics.hpp"
#include "extensions/cpp_defines.hpp"

namespace fi::gfx
{
    struct tex_arr : private graphcis_obj
    {
        vk::Fence fence_;

        std::vector<vk::Image> images_{};
        std::vector<vk::ImageView> views_{};
        std::vector<vma::Allocation> allocs_{};
        std::vector<vk::Sampler> samplers_{};
        std::vector<vk::DescriptorImageInfo> desc_infos_{};

        tex_arr();
        ~tex_arr();

        // possibly layers
        void add_sampler(const vk::SamplerCreateInfo& sampler_info);
        void add_tex(vk::CommandPool cmd_pool,
                     uint32_t sampler_idx,
                     const std::vector<std::byte>& data,
                     vk::Extent3D extent,
                     uint32_t levels = 1);
    };
}; // namespace fi::gfx

#endif // GRAPHICS_TEXTURES_HPP