#ifndef ENGINE_SCENE_HPP
#define ENGINE_SCENE_HPP

#include "res_uploader.hpp"

namespace fi
{
    struct ResSceneNode
    {
        size_t depth_ = 0;
        size_t parent_idx = -1;
        size_t node_idx = 0;
        std::string name_ = "";

        glm::vec3 translation_ = {0, 0, 0};
        glm::quat rotation_ = {0, 0, 0, 1};
        glm::vec3 scale_ = {1, 1, 1};
        glm::mat4 preset_ = glm::identity<glm::mat4>(); // set by ResSceneDetails
    };

    // support only 1 scene
    struct ResSceneDetails : private GraphicsObject
    {
      private:
        std::vector<ResSceneNode> nodes_{};                // indexed by node, cleared
        std::vector<std::vector<size_t>> node_children_{}; // indexed by node, cleared
        std::vector<std::vector<size_t>> node_layers_{};   // cleared

        std::vector<ResSceneNode> nodes2_{};   // indexed by nodes2_mapping_[node]
        std::vector<size_t> nodes2_mapping_{}; // indexed by node

        void build_scene_layer(size_t curr);

      public:
        struct SceneOffsets
        {
            vk::DeviceSize node_transform_idx_ = 0;
            vk::DeviceSize node_transform_ = 0;
        };

        std::string name_ = "";

        std::array<vk::DescriptorPoolSize, 1> des_sizes_{};
        vk::DescriptorSetLayout set_layout_{};
        vk::DescriptorSet des_set_{}; // bind to vertex shader

        std::unique_ptr<Buffer<SceneOffsets, storage, seq_write>> buffer_{};
        std::vector<uint32_t> node_transform_idx_{}; // indexed by prim
        std::vector<glm::mat4> node_transform_{};
        // indexed by node and node_transform_idx_[prim], calculated by update_scene()

        std::vector<size_t> roots_;

        ResSceneDetails(const ResDetails& res_details);
        ~ResSceneDetails();

        // calculate node_transform after the callback
        void update_data();
        inline ResSceneNode& index_node(size_t node_idx) { return nodes2_[nodes2_mapping_[node_idx]]; }
        inline size_t node_size() { return nodes2_.size(); }
        void update_scene(const std::function<void(ResSceneNode& node, size_t node_idx)>& func,
                          const glm::mat4& root_transform = glm::identity<glm::mat4>());
        void update_scene(const glm::mat4& root_transform = glm::identity<glm::mat4>());
        void allocate_descriptor(vk::DescriptorPool des_pool);
        void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipeline_layout, uint32_t set);
    };

    template <typename OutT>
    struct ResAnimationSampler
    {
        size_t interporlation_method_ = 0;
        std::vector<float> time_stamps_{};
        std::vector<OutT> output_{};

        OutT sample_time_stamp(float time_stamp, const OutT& alternative);
    };

    struct ResKeyFrame
    {
        glm::vec3 translation_{};
        glm::quat rotation_{};
        glm::vec3 scale_{};
    };

    struct ResKeyFrames
    {
        ResAnimationSampler<glm::vec3> translation_sampler_;
        ResAnimationSampler<glm::quat> rotation_sampler_;
        ResAnimationSampler<glm::vec3> scale_sampler_;

        ResKeyFrame sample_time_stamp(float time_stamp);
        void set_sample_time_stamp(float time_stamp, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);
        void set_sample_time_stamp(float time_stamp, ResSceneNode& node);
    };

    struct ResAnimation
    {
        std::string name_ = "";
        std::vector<size_t> key_frames_idx_{};   // indexed by node
        std::vector<ResKeyFrames> key_frames_{}; // indexed by key_frames_idx_[node]
    };

    std::vector<ResAnimation> load_res_animations(const ResDetails& res_details);

    template <typename OutT>
    inline OutT ResAnimationSampler<OutT>::sample_time_stamp(float time_stamp, const OutT& alternative)
    {
        if (output_.empty())
        {
            return alternative;
        }

        OutT result = output_[0];
        float curr_time = time_stamp;
        float prev_time = 0;
        float next_time = 0;
        const OutT* prev_out = &result;
        const OutT* next_out = nullptr;

        if (curr_time >= time_stamps_.back())
        {
            int quot = curr_time / time_stamps_.back();
            curr_time -= quot * time_stamps_.back();
        }

        for (size_t i = 0; i < time_stamps_.size(); i++)
        {
            if (time_stamps_[i] >= curr_time)
            {
                next_time = time_stamps_[i];
                next_out = &output_[i];
                if (i)
                {
                    prev_time = time_stamps_[i - 1];
                    prev_out = &output_[i - 1];
                }
                break;
            }
        }

        if (next_out == nullptr)
        {
            return *prev_out;
        }

        switch (interporlation_method_)
        {
            case 0: // linear
            case 2: // cubic spine not supported
            {
                float inpl_val = (curr_time - prev_time) / (next_time - prev_time);
                if constexpr (std::is_same_v<OutT, glm::quat>)
                {
                    result = glm::slerp(*prev_out, *next_out, inpl_val);
                }
                else
                {
                    result = glm::lerp(*prev_out, *next_out, inpl_val);
                }
                break;
            }
            case 1: // steps
            {
                result = *prev_out;
                break;
            }
        }
        return result;
    }
}; // namespace fi

#endif // ENGINE_SCENE_HPP
