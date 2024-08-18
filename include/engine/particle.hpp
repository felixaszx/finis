#ifndef ENGINE_PARTICLE_HPP
#define ENGINE_PARTICLE_HPP

#include "graphics/graphics.hpp"
#include "graphics/res_loader.hpp"
#include "graphics/buffer.hpp"

namespace fi
{
    class ParticleGroup
    {
      public:
        struct alignas(16) Particle
        {
            glm::vec4 position_ = {0, 0, 0, 1}; //[3] == place holder
            glm::vec4 velocity_ = {0, 0, 0, 1}; // [3] == linear acceleration factor
            /**
             * @brief Actuall velocity = sum of all Target.position_ - position + dt * velocity_[3] * velocity.xyz;
             * There do not exist a life time for any of the particles. Particles are constantly moving. Or until
             * the compute shader stops.
             */
        };

        struct alignas(16) Target // update by writing
        {
            enum Mode : uint32_t
            {
                TOWORD, // Moving toword the adjustment
                FOLLOW, // Moving follow the adjustment as direction
                AWAY    // Moving away from the adjustment
            };

            glm::vec4 adjustment_ = {0, 0, 0, 1}; // [3] use as mode
            glm::vec4 color_ = {1, 1, 1, 1};      // color apply to particle

            void set_target_mode(Mode mode) { adjustment_[3] = mode; }
        };

      private:
        std::vector<Particle> particles_{}; // binding 0
        std::vector<Target> targets_{};     // binding 1

      public:
        // buffer
        struct BufferOffsetP
        {
            vk::DeviceSize particles_{};
            vk::DeviceSize targets_{};
        };

        std::unique_ptr<Buffer<BufferBase::EmptyExtraInfo, vertex, storage>> buffer_{};

        // descriptors
        std::array<vk::DescriptorPoolSize, 2> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{};

        ParticleGroup(uint32_t max_particles);
        ParticleGroup(vk::Buffer positions, vk::DeviceSize offset, vk::DeviceSize range); // in vec3 only
        ~ParticleGroup();

        void bind_res(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t des_set);
        void draw(vk::CommandBuffer cmd);
    };
}; // namespace fi

#endif // ENGINE_PARTICLE_HPP