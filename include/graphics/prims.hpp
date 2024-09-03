#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

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
        vk::DeviceSize load_staging_memory(const std::byte* data, vk::DeviceSize size);
        void free_staging_buffer();
        uint32_t add_primitives(const std::vector<vk::DrawIndirectCommand>& draw_calls);
        void end_primitives();
        void reload_draw_calls(vk::CommandPool pool);
        [[nodiscard]] vk::DeviceSize addresses_size() const { return sizeof(addresses_); }
        [[nodiscard]] const std::byte* addresses() const { return castr(const std::byte*, &addresses_); }

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
            size_t offset = load_staging_memory(castr(const std::byte*, data.data()), sizeof_arr(data));
            if (offset == -1)
            {
                flush_staging_memory(pool);
                offset = load_staging_memory(castr(const std::byte*, data.data()), sizeof_arr(data));
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
            size_t offset = load_staging_memory(castr(const std::byte*, data.data()), sizeof_arr(data));
            if (offset == -1)
            {
                flush_staging_memory(pool);
                offset = load_staging_memory(castr(const std::byte*, data.data()), sizeof_arr(data));
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

    struct prim_structures : private graphcis_obj
    {
        struct node_trs
        {
            std::string name_;
            glm::mat4* transform_ = nullptr;
            uint32_t weight_count_ = 0;
            float* wieghts_ = nullptr;
            node_trs* parent_ = nullptr;

            glm::vec3 t_ = {0, 0, 0};
            glm::quat r_ = {0, 0, 0, 1};
            glm::vec3 s_ = {1, 1, 1};
            glm::mat4 preset_ = glm::identity<glm::mat4>();
        };

        struct
        {
            vk::DeviceSize curr_size_ = 0;
            const vk::DeviceSize capacity_ = 0;

            vk::Buffer buffer_{};
            vma::Allocation alloc_{};
        } data_; // buffer 0

        std::vector<node_trs> nodes_{};

        std::vector<uint32_t> meshe_idxs_{}; // the size of primitive
        std::vector<mesh_info> meshes_{};
        std::vector<float> morph_weights_{};
        std::vector<glm::mat4> tranforms_{};

        prim_structures(uint32_t prim_count);
        ~prim_structures();
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PRIMS_HPP