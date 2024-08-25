#include "graphics/prims.hpp"

fi::Primitives::Primitives(uint32_t max_data_size, uint32_t max_prim_count)
    : max_size_(max_data_size),
      max_prims_(max_prim_count)
{
    setter_.prims_ = this;
}

fi::Primitives::~Primitives() {}
