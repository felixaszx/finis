/**
 * @file res_skin.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2024-08-15
 * 
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 * 
 */
#ifndef GRAPHICS_RES_SKIN_HPP
#define GRAPHICS_RES_SKIN_HPP

#include "res_loader.hpp"
#include "res_structure.hpp"

namespace fi
{
    class ResSkinDetails : private GraphicsObject
    {
      public:
        struct SkinInfo
        {
            TSNodeIdx first_joint_{};
            uint32_t joint_count_ = EMPTY;
        };

      private:
        struct BufferOffsets
        {
            vk::DeviceSize joints_ = 0;
            vk::DeviceSize inv_binds_ = 0;
        };

        // helpers
        std::vector<size_t> first_skin_{};
        std::vector<SkinInfo> skins_{};

      public:
        // descriptors
        std::array<vk::DescriptorPoolSize, 1> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{};

        std::vector<uint32_t> joints_{};     // binding 0
        std::vector<glm::mat4> inv_binds_{}; // binding 1
        std::unique_ptr<Buffer<BufferOffsets, storage>> buffer_;

        ResSkinDetails(ResDetails& res_details, ResStructure& res_structure);
        ~ResSkinDetails();

        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind_res(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set);
    };
}; // namespace fi

#endif // GRAPHICS_RES_SKIN_HPP
