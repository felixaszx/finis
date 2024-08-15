#ifndef GRAPHICS_RES_ANIM_HPP
#define GRAPHICS_RES_ANIM_HPP

#include "res_loader.hpp"
#include "res_structure.hpp"

namespace fi
{
    struct CombinedAnimChannel
    {
        TSNodeIdx node_idx_{};
        uint32_t weights_count_ = EMPTY;
        float* weights_ = nullptr;
        ResStructure::NodeInfo* node_ref_ = nullptr;

        std::vector<float> translation_timeline_{};
        std::vector<float> rotation_timeline_{};
        std::vector<float> scale_timeline_{};
        std::vector<float> weight_timeline_{};

        std::vector<glm::vec3> translation_samples_{};
        std::vector<glm::quat> rotation_samples_{};
        std::vector<glm::vec3> scale_samples_{};
        std::vector<float> weight_samples_{};

        void sample_animation(float time_point = 0);
    };

    struct ResAnimation
    {
        std::string name_ = "";
        // storages
        std::vector<CombinedAnimChannel> channels_{};

        void set_keyframe(float time_point = 0);
    };

    std::vector<ResAnimation> get_res_animations(ResDetails& res_details,     //
                                                 ResStructure& res_structure, //
                                                 size_t gltf_idx);
}; // namespace fi

#endif // GRAPHICS_RES_ANIM_HPP