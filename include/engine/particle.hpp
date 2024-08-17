#ifndef ENGINE_PARTICLE_HPP
#define ENGINE_PARTICLE_HPP

#include "graphics/graphics.hpp"
#include "graphics/res_loader.hpp"
#include "graphics/buffer.hpp"

namespace fi
{
    class ParticleSystem
    {
      private:
        std::vector<glm::vec3> positios_{};
        std::vector<glm::vec3> velocities_{};
        std::vector<float> alpha_cut_offs_{};
        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, vertex, storage>> buffer_{};

      public:
        ParticleSystem(uint32_t max_particles);
        ~ParticleSystem();
    };
}; // namespace fi

#endif // ENGINE_PARTICLE_HPP
