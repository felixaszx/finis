#ifndef GRAPHICS_RENDER_TARGET_HPP
#define GRAPHICS_RENDER_TARGET_HPP

#include "graphics.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "pipeline.hpp"
#include "glms.hpp"

namespace fi
{
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

        static std::vector<vk::VertexInputBindingDescription> vtx_bindings();
        static std::vector<vk::VertexInputAttributeDescription> vtx_attributes();
    };

    class AnimationMgr;
    class RenderMgr : private GraphicsObject
    {
        friend AnimationMgr;

      public:
        struct VtxIdxBufferExtra
        {
            // vertex buffer
            vk::DeviceSize vtx_offset_ = 0;

            // index buffer
            vk::DeviceSize idx_offset_ = 0;

            // storage buffer
            vk::DeviceSize mat_offset_ = 0;
            vk::DeviceSize mat_idx_offset_ = 0;
        };

        struct HostBufferExtra
        {
            // indirect buffer
            vk::DeviceSize draw_call_offset_ = 0;

            // storage buffer
            vk::DeviceSize bone_offset_ = 0;
        };

      private:
        bool locked_ = false;
        gltf::Parser parser_;
        vk::DescriptorPool pool_;

        // access below via Renderable::data_idx_
        std::vector<std::vector<uint32_t>> mat_idxs_{};
        std::vector<std::vector<vk::DrawIndexedIndirectCommand>> draw_calls_{};
        std::vector<Buffer<VtxIdxBufferExtra, vertex, index, storage>> device_buffers_{};
        std::vector<Buffer<HostBufferExtra, indirect, storage, seq_write>> host_buffers_{};

        // texture storage
        std::vector<vk::Sampler> sampelers_{};                              // not for access
        std::vector<std::vector<Material>> materials_{};                    // read only
        std::vector<std::vector<vk::DescriptorImageInfo>> texture_infos_{}; // read only
        std::vector<vk::DescriptorSetLayout> texture_set_layouts_{};
        std::vector<vk::DescriptorSet> texture_sets_{};

      public:
        using DataIdx = size_t;
        ~RenderMgr();

        [[nodiscard]] std::vector<vk::DescriptorSetLayout> texture_set_layouts() const;
        Renderable get_renderable(DataIdx data_idx, size_t renderable_idx);
        std::pair<fi::RenderMgr::DataIdx, size_t> upload_res(const std::filesystem::path& path, //
                                                             TextureMgr& texture_mgr, AnimationMgr& animation_mgr,
                                                             gltf::Expected<gltf::GltfDataBuffer>& gltf_file);
        void lock_and_prepared();
        void draw(const std::vector<DataIdx>& draws,
                  const std::function<void(vk::Buffer device_buffer, //
                                           uint32_t vtx_buffer_binding,
                                           const VtxIdxBufferExtra& offsets, //
                                           vk::Buffer host_buffer,
                                           const HostBufferExtra& host_offsets, //
                                           vk::DescriptorSet texture_set)>& draw_func);
    };

}; // namespace fi

#endif // GRAPHICS_RENDER_TARGET_HPP
