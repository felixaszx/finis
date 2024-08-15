#ifndef ENGINE_PARTICLE_HPP
#define ENGINE_PARTICLE_HPP

#include "graphics/graphics.hpp"
#include "graphics/res_loader.hpp"
#include "graphics/buffer.hpp"

namespace fi
{
    struct alignas(16) ParticleInfo
    {
    };

    class ParticleSystem
    {
      private:
        PrimIdx prim_idx_;
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, vertex, storage>> buffer_{};

      public:
    };
}; // namespace fi

#endif // ENGINE_PARTICLE_HPP
