#ifndef ENGINE_RESOURCE_HPP
#define ENGINE_RESOURCE_HPP

#include "graphics/res_loader.hpp"
#include "graphics/res_structure.hpp"
#include "graphics/res_anim.hpp"
#include "graphics/res_skin.hpp"

namespace fi
{
    struct InstancingInfo
    {
        InstanceIdx first_matrix_ = EMPTY;
        uint32_t instance_count_ = 0;
        uint32_t max_instance_ = 0;
    };

    struct SceneRenderableRef
    {
        glm::mat4* matrix_ = nullptr;
    };

    struct SceneRenderable
    {
        SharedObj<uint32_t> avaliable_instances_{nullptr};
        InstancingInfo* instancing_info_ = nullptr;
        std::vector<SceneRenderableRef> refs_{};
    };

    class SceneResources : private GraphicsObject
    {
      private:
        struct BufferOffsets
        {
            vk::DeviceSize instancing_infos_ = 0;
            vk::DeviceSize instancing_matrices_ = 0;
        };

        SharedObj<uint32_t> avaliable_instances_ = 0;
        std::vector<SceneRenderable> renderables_{};

      public:
        UniqueObj<ResDetails> res_detail_;
        UniqueObj<ResStructure> res_structure_{nullptr};
        UniqueObj<ResSkinDetails> res_skin_{nullptr};
        std::vector<std::vector<ResAnimation>> res_anims_{};

        std::array<vk::DescriptorPoolSize, 1> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{};

        // storage
        std::vector<InstancingInfo> instancing_infos_{};
        std::vector<glm::mat4> instancing_matrices_{};
        UniqueObj<Buffer<BufferOffsets, storage, seq_write, host_coherent>> buffer_{nullptr};

        std::filesystem::path res_path_ = "";

        SceneResources(const std::filesystem::path& res_path, uint32_t max_instances = 0);

        void update_data();
        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind_res(vk::CommandBuffer cmd,
                      vk::PipelineBindPoint bind_point, //
                      vk::PipelineLayout pipeline_layout,
                      uint32_t des_set);
        void compute(vk::CommandBuffer cmd, const glm::uvec3& work_group = {0, 0, 0}); 
    };
}; // namespace fi

#endif // ENGINE_RESOURCE_HPP
