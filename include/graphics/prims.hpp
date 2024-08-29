#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

#include "graphics.hpp"
#include "tools.hpp"
#include "prim_data.hpp"
#include "circular_span.hpp"

namespace fi::graphics
{
    struct Primitives : private GraphicsObject
    {
      private:
        // min alignment in buffer is 16 byte
        struct
        {
            vk::DeviceSize curr_size_ = 0;
            const vk::DeviceSize capacity_ = 0;

            vk::Buffer buffer_{};
            vma::Allocation alloc_{};
        } data_; // buffer 0

        struct
        {
            uint32_t count_ = 0;
            const uint32_t capacity_ = 0;

            vk::Buffer buffer_{}; // buffer 1
            vma::Allocation alloc_{};
            vk::DeviceSize draw_call_offset_ = EMPTY_L;
        } prims_;
        uint32_t curr_prim_ = EMPTY;
        std::vector<PrimInfo> prim_infos_{};
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

      public:
        Primitives(vk::DeviceSize data_size_limit, uint32_t prim_limit);
        ~Primitives();

        void generate_staging_buffer(vk::DeviceSize limit);
        void flush_staging_memory(vk::CommandPool pool);
        vk::DeviceSize load_staging_memory(const std::byte* data, vk::DeviceSize size);
        void free_staging_buffer();
        uint32_t add_primitives(const std::vector<vk::DrawIndirectCommand>& draw_calls);
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
        Primitives& add_attribute_data(vk::CommandPool pool,
                                       PrimInfo::Attribute attrib,
                                       const T& data,
                                       const std::vector<size_t>& offset_per_prim = {})
        {
            size_t offset = load_staging_memory(castr(const std::byte*, data.data()), sizeof_arr(data));
            if (offset == EMPTY_L)
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
            return *this;
        }

        template <typename T>
        Primitives& load_morph_data(vk::CommandPool pool,
                                    MorphInfo::Attribute attrib,
                                    const T& data,
                                    std::vector<MorphInfo>& infos,
                                    const std::vector<int64_t>& morph_count,
                                    const std::vector<size_t>& offset_per_info = {})
        {
            size_t offset = load_staging_memory(castr(const std::byte*, data.data()), sizeof_arr(data));
            if (offset == EMPTY_L)
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
            return *this;
        }
    };
}; // namespace fi::graphics

#endif // GRAPHICS_PRIMS_HPP