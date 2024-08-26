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
      private:
        // min alignment in data_buffer is 16 byte
        struct
        {
            vk::DeviceSize curr_size_ = 0;
            const vk::DeviceSize capacity_ = 0;

            vk::Buffer buffer_{};
            vma::Allocation alloc_{};
        } data_; // buffer 0
        vk::DeviceSize idx_buffer_offset_ = EMPTY_L;

        struct
        {
            uint32_t count_ = 0;
            const uint32_t capacity_ = 0;

            vk::Buffer buffer_{}; // buffer 1
            vma::Allocation alloc_{};
            vk::DeviceSize draw_call_offset_ = EMPTY_L;
        } prims_;

        struct
        {
            vk::DeviceAddress data_buffer_ = 0;
            vk::DeviceAddress prim_buffer_ = 0;
        } addresses_{};

        vk::Buffer staging_buffer_{};
        vma::Allocation staging_alloc_{};
        CircularSpan staging_span_{};

      public:
        Primitives(vk::DeviceSize data_size_limit, uint32_t prim_limit);
        void generate_staging_buffer(vk::DeviceSize limit);
        ~Primitives();
    };
}; // namespace fi::graphics

#endif // GRAPHICS_PRIMS_HPP