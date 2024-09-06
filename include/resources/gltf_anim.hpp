#ifndef RESOURCES_GLTF_ANIM_HPP
#define RESOURCES_GLTF_ANIM_HPP

#include "gltf_file.hpp"

namespace fi::res
{
    struct gltf_frame // linear interporlation only
    {
        std::vector<float> time_t_{};
        std::vector<glm::vec3> t_{};
        std::vector<float> time_r_{};
        std::vector<glm::quat> r_{};
        std::vector<float> time_s_{};
        std::vector<glm::vec3> s_{};

        std::vector<float> time_w_{};
        uint32_t w_count_ = 0;
        std::vector<float> w_{};

        glm::mat4* t_out_ = nullptr;
        glm::mat4* r_out_ = nullptr;
        glm::mat4* s_out_ = nullptr;
        float* w_out_ = nullptr;

        void sample_kframe(float time_p);
        bool empty() { return time_t_.empty() && time_r_.empty() && time_s_.empty() && time_w_.empty(); }
    };

    struct gltf_anim
    {
        std::string name_ = "";
        std::vector<gltf_frame> frames_; // the size of gltf nodes

        gltf_anim(const gltf_file& file, uint32_t index = 0);
        ~gltf_anim() = default;
    };
}; // namespace fi::res

#endif // RESOURCES_GLTF_ANIM_HPP