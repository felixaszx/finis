#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

#include <ranges>

#include "graphics.hpp"
#include "tools.hpp"
#include "prim_data.hpp"
#include "circular_span.hpp"

namespace fi::gfx
{
    struct primitives : private graphcis_obj
    {
        struct
        {
            vk::DeviceSize curr_size_ = 0;
            const vk::DeviceSize capacity_ = 0;

            vk::Buffer buffer_{}; // buffer 0
            vma::Allocation alloc_{};
        } data_;

        struct
        {
            uint32_t count_ = 0;
            const uint32_t capacity_ = 0;

            vk::Buffer buffer_{}; // buffer 1
            vma::Allocation alloc_{};
            vk::DeviceSize draw_call_offset_ = -1;
        } prims_;
        uint32_t curr_prim_ = -1;
        std::vector<prim_info> prim_infos_{};
        std::vector<vk::DrawIndirectCommand> draw_calls_{};

        struct
        {
            vk::DeviceAddress data_buffer_ = 0;
            vk::DeviceAddress prim_buffer_ = 0;
        } addresses_{};

        vk::Buffer staging_buffer_{};
        vma::Allocation staging_alloc_{};
        circular_span staging_span_{};
        std::queue<vk::DeviceSize> staging_queue_{};

        primitives(vk::DeviceSize data_size_limit, uint32_t prim_limit);
        ~primitives();

        void generate_staging_buffer(vk::DeviceSize limit);
        void flush_staging_memory(vk::CommandPool pool);
        vk::DeviceSize write_staging_memory(const std::byte* data, vk::DeviceSize size);
        void free_staging_buffer();
        uint32_t add_primitives(const std::vector<vk::DrawIndirectCommand>& draw_calls);
        void end_primitives();
        void reload_draw_calls(vk::CommandPool pool);
        [[nodiscard]] vk::DeviceSize addresses_size() const { return sizeof(addresses_); }
        [[nodiscard]] const std::byte* addresses() const { return util::castr<const std::byte*>(&addresses_); }

        template <typename T>
        static std::vector<size_t> get_elm_offset_per_prim(const T& data)
        {
            std::vector<size_t> offsets(data.size());
            offsets[0] = 0;

            auto offset_iter = offsets.begin() + 1;
            while (offset_iter != offsets.end())
            {
                *offset_iter = *(offset_iter - 1) + sizeof(data[0]);
            }
        }

        // below loaders do not include range for better performance
        template <typename T>
        void add_attribute_data(vk::CommandPool pool,
                                prim_info::attrib attrib,
                                const T& data,
                                const std::vector<size_t>& offset_per_prim = {})
        {
            if (sizeof_arr(data) == 0)
            {
                return;
            }
            size_t offset = write_staging_memory(util::castr<const std::byte*>(data.data()), util::sizeof_arr(data));
            if (offset == -1)
            {
                flush_staging_memory(pool);
                offset = write_staging_memory(util::castr<const std::byte*>(data.data()), util::sizeof_arr(data));
            }

            size_t i = 0;
            for (; i < offset_per_prim.size(); i++)
            {
                prim_infos_[curr_prim_ + i].get_attrib(attrib) = offset + offset_per_prim[i];
            }
            for (i += curr_prim_; i < prim_infos_.size(); i++)
            {
                prim_infos_[i].get_attrib(attrib) = offset;
            }
        }

        template <typename T>
        void load_morph_data(vk::CommandPool pool,
                             morph_info::attrib attrib,
                             const T& data,
                             std::vector<morph_info>& infos,
                             const std::vector<int64_t>& morph_count,
                             const std::vector<size_t>& offset_per_info = {})
        {
            if (sizeof_arr(data) == 0)
            {
                return;
            }
            size_t offset = write_staging_memory(util::castr<const std::byte*>(data.data()), util::sizeof_arr(data));
            if (offset == -1)
            {
                flush_staging_memory(pool);
                offset = write_staging_memory(util::castr<const std::byte*>(data.data()), util::sizeof_arr(data));
            }

            size_t i = 0;
            for (; i < offset_per_info.size(); i++)
            {
                infos[i].set_attrib(attrib, morph_count[i]) = offset + offset_per_info[i];
            }
            for (; i < infos.size(); i++)
            {
                infos[i].set_attrib(attrib, morph_count[i]) = offset;
            }
        }
    };

    struct prim_structure : private graphcis_obj
    {
        struct data
        {
            vk::Buffer buffer_{};
            vma::Allocation alloc_{};
            std::byte* mapping_ = nullptr;

            struct uniforms
            {
                vk::DeviceAddress address_ = 0;
                vk::DeviceSize mesh_idx_offset_ = 0;
                vk::DeviceSize meshes_offset_ = 0;
                vk::DeviceSize morph_weights_offset_ = 0;
                vk::DeviceSize tranforms_offset_ = 0;
            } uniforms_;

            void copy_to(std::byte* dst) { memcpy(dst, &uniforms_, sizeof(uniforms_)); }
        } data_; // buffer 0

        std::vector<node_trs> nodes_{}; // update sequentially

        std::vector<uint32_t> mesh_idxs_{}; // the size of primitive
        std::vector<mesh_info> meshes_{};
        std::vector<float> morph_weights_{};
        std::vector<glm::mat4> tranforms_{};

        prim_structure(uint32_t prim_count);
        ~prim_structure();

        void load_data();
        void reload_data();
        // transform will not be applied to nodes that has parent
        void process_nodes(const glm::mat4& transform = glm::identity<glm::mat4>());
        void add_mesh(const std::vector<uint32_t>& prim_idx,
                      uint32_t node_idx,
                      uint32_t transform_idx,
                      const node_trs& trs = {});
        void set_mesh_morph_weights(uint32_t mesh_idx, const std::vector<float>& weights);
    };

    struct prim_skins : private graphcis_obj
    {
        struct data
        {
            vk::Buffer buffer_{};
            vma::Allocation alloc_{};

            vk::DeviceAddress address_ = 0;
            uint64_t joints_offset_ = -1;
            uint64_t inv_binds_offset_ = -1;
        } data_; // buffer 0

        std::vector<uint32_t> skin_offsets_;

        std::vector<uint32_t> mesh_skin_idxs_{}; // the size of mesh, access from prim_structure
        std::vector<uint32_t> joints_{};         // index to node from prim_structure
        std::vector<glm::mat4> inv_binds_{};     // for each joint

        prim_skins(uint32_t mesh_count);
        ~prim_skins();

        void add_skin(const std::vector<uint32_t>& new_joints, const std::vector<glm::mat4>& new_inv_binds);
        void load_data(vk::CommandPool pool);
        void set_skin(uint32_t mesh_idx, uint32_t skin_idx);
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PRIMS_HPP