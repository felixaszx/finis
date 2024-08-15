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

        std::vector<CpuClock::MilliSecond> translation_timeline_{}; // in ms
        std::vector<CpuClock::MilliSecond> rotation_timeline_{};
        std::vector<CpuClock::MilliSecond> scale_timeline_{};
        std::vector<CpuClock::MilliSecond> weight_timeline_{};

        std::vector<glm::vec3> translation_samples_{};
        std::vector<glm::quat> rotation_samples_{};
        std::vector<glm::vec3> scale_samples_{};
        std::vector<float> weight_samples_{};

        void sample_animation(CpuClock::MilliSecond time_point = 0, bool stepping = false);
    };

    struct ResAnimation
    {
        std::string name_ = "";
        // storages
        std::vector<CombinedAnimChannel> channels_{};

        void set_keyframe(CpuClock::MilliSecond time_point = 0,bool stepping = false);
    };

    std::vector<ResAnimation> get_res_animations(ResDetails& res_details,     //
                                                 ResStructure& res_structure, //
                                                 size_t gltf_idx);
}; // namespace fi

#endif // GRAPHICS_RES_ANIM_HPP