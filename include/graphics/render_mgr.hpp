#ifndef GRAPHICS_RENDER_TARGET_HPP
#define GRAPHICS_RENDER_TARGET_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "glms.hpp"

namespace fi
{
    struct Material
    {
        glm::vec4 color_factor_ = {1, 1, 1, 1}; // alignment
        float metalic_ = 1.0f;
        float roughtness_ = 1.0f;
        uint32_t color_texture_idx_{};
        uint32_t metalic_roughtness_{}; // alignment
    };

    struct Renderable
    {
        struct Vertex
        {
            glm::vec3 position_{};     // 0
            glm::vec3 normal_{};       // 1
            glm::vec2 tex_coord_{};    // 2
            glm::uvec4 bone_ids_{};    // 3
            glm::vec4 bone_weights_{}; // 4
        };

        Material* mat_ = nullptr;
        vk::DrawIndexedIndirectCommand* draw_call_ = nullptr;
    };

    class RenderMgr : private GraphicsObject
    {
      private:
        struct VtxIdxBufferExtra
        {
            // vertex buffer
            vk::DeviceSize vtx_offset_ = 0;

            // index buffer
            vk::DeviceSize idx_offset_ = 0;

            // storage buffer
            vk::DeviceSize mat_offset_ = 0;
            vk::DeviceSize bone_offset_ = 0;

            // indirect draw buffer
            vk::DeviceSize draw_call_offset_ = 0;
        };

        bool locked_ = false;
        gltf::Parser parser_;
        vk::DescriptorPool pool_;

        // access below via Renderable::data_idx_
        std::vector<vk::Sampler> sampelers_{};                              // not for access
        std::vector<std::vector<Material>> materials_{};                    // read only
        std::vector<std::vector<vk::DescriptorImageInfo>> texture_infos_{}; // read only
        std::vector<std::vector<vk::DrawIndexedIndirectCommand>> draw_calls_{};
        std::vector<Buffer<VtxIdxBufferExtra, vertex, index, storage, indirect>> device_buffers_{};

        // texture storage
        std::vector<vk::DescriptorSetLayout> texture_set_layouts_{};
        std::vector<vk::DescriptorSet> texture_sets_{};

      public:
        using DataIdx = size_t;
        ~RenderMgr();

        [[nodiscard]] std::vector<vk::DescriptorSetLayout> texture_set_layouts() const;
        Renderable get_renderable(DataIdx data_idx, size_t renderable_idx);
        std::pair<DataIdx, size_t> upload_res(const std::filesystem::path& path, TextureMgr& texture_mgr);
        void lock_and_prepared();
        void draw(const std::vector<DataIdx>& draws,
                  const std::function<void(vk::Buffer device_buffer, const VtxIdxBufferExtra&,
                                           vk::DescriptorSet texture_set)>& draw_func);
    };

}; // namespace fi

#endif // GRAPHICS_RENDER_TARGET_HPP
