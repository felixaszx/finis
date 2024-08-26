#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

#include "graphics.hpp"
#include "tools.hpp"
#include "prim_data.hpp"

namespace fi
{
    struct Primitives : private GraphicsObject
    {
      private:
      private:
        // min alignment in data_buffer is 16 byte
        struct
        {
            vk::DeviceSize curr_size_ = 0;
            const vk::DeviceSize max_size_ = 0;
            
            vk::Buffer buffer_{};
            vma::Allocation alloc_{};
        } data_; // buffer 0
        vk::DeviceSize idx_buffer_offset_ = EMPTY_L;

        struct
        {
            uint32_t prim_count_ = 0;
            const uint32_t max_prims_ = 0;

            vk::Buffer buffer_{}; // buffer 1
            vma::Allocation alloc_{};
            vk::DeviceSize draw_call_offset_ = EMPTY_L;
        } prims_;

        struct
        {
            vk::DeviceAddress data_buffer_ = 0;
            vk::DeviceAddress prim_buffer_ = 0;
        } addresses_{};

      public:
        Primitives(vk::DeviceSize max_data_size, uint32_t max_prim_count);
        ~Primitives();
    };
}; // namespace fi

#endif // GRAPHICS_PRIMS_HPP