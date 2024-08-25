#ifndef GRAPHICS_PRIMS_HPP
#define GRAPHICS_PRIMS_HPP

#include "graphics.hpp"
#include "prim_data.hpp"

namespace fi
{
    struct Primitives
    {
        // min offset in data_buffer is 4 byte
        vk::DeviceSize curr_size_ = 0;
        vk::DeviceSize max_size_ = 0;
        vk::Buffer data_buffer_{};
        vk::DeviceAddress data_address_{};
        vma::Allocation data_alloc_{};

        uint32_t prim_count_ = 0;
        uint32_t max_prims_ = 0;
        vk::Buffer prim_buffer_{};
        vk::DeviceAddress prim_address_{};
        vma::Allocation prim_alloc_{};
    };
}; // namespace fi

#endif // GRAPHICS_PRIMS_HPP