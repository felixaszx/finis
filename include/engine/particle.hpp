#ifndef ENGINE_PARTICLE_HPP
#define ENGINE_PARTICLE_HPP

#include "graphics2/graphics.hpp"
#include "graphics2/res_uploader.hpp"
#include "graphics2/buffer.hpp"

namespace fi
{
    struct alignas(16) ParticleInfo
    {
    };

    class ParticleSystem
    {
      private:
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, vertex, storage>> buffer_{};

      public:
    };
}; // namespace fi

#endif // ENGINE_PARTICLE_HPP
