#include "resources/gltf_anim.hpp"

fi::res::gltf_anim::gltf_anim(const gltf_file& file, uint32_t index)
{
    name_ = file.asset_->animations[index].name;

    const fgltf::Animation& anim = file.asset_->animations[index];
    for (const auto& channel : anim.channels)
    {
        const fgltf::AnimationSampler& sampler = anim.samplers[channel.samplerIndex];
        if (channel.nodeIndex >= frames_.size())
        {
            frames_.resize(*channel.nodeIndex + 1);
        }

        const fgltf::Accessor& input_acc = file.asset_->accessors[sampler.inputAccessor];
        const fgltf::Accessor& output_acc = file.asset_->accessors[sampler.outputAccessor];

        switch (channel.path)
        {
            case fgltf::AnimationPath::Translation:
            {
                frames_[*channel.nodeIndex].time_t_.reserve(input_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, input_acc,
                                              [&](float time) { frames_[*channel.nodeIndex].time_t_.push_back(time); });
                frames_[*channel.nodeIndex].t_.reserve(output_acc.count);
                fgltf::iterateAccessor<glm::vec3>(*file.asset_, output_acc, [&](const glm::vec3& t)
                                                  { frames_[*channel.nodeIndex].t_.push_back(t); });
                break;
            }
            case fgltf::AnimationPath::Rotation:
            {
                frames_[*channel.nodeIndex].time_r_.reserve(input_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, input_acc,
                                              [&](float time) { frames_[*channel.nodeIndex].time_r_.push_back(time); });
                frames_[*channel.nodeIndex].r_.reserve(output_acc.count);
                fgltf::iterateAccessor<fgltf::math::quat<float>>(
                    *file.asset_, output_acc, [&](const fgltf::math::quat<float>& r)
                    { glms::assign_value(frames_[*channel.nodeIndex].r_.emplace_back(), r); });
                break;
            }
            case fgltf::AnimationPath::Scale:
            {
                frames_[*channel.nodeIndex].time_s_.reserve(input_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, input_acc,
                                              [&](float time) { frames_[*channel.nodeIndex].time_s_.push_back(time); });
                frames_[*channel.nodeIndex].s_.reserve(output_acc.count);
                fgltf::iterateAccessor<glm::vec3>(*file.asset_, output_acc, [&](const glm::vec3& s)
                                                  { frames_[*channel.nodeIndex].s_.push_back(s); });
                break;
            }
            case fgltf::AnimationPath::Weights:
            {
                uint32_t wieght_count = file.asset_->nodes[*channel.nodeIndex].weights.size();
                if (wieght_count == 0)
                {
                    wieght_count = file.asset_ //
                                       ->meshes[*file.asset_->nodes[*channel.nodeIndex].meshIndex]
                                       .weights.size();
                }
                frames_[*channel.nodeIndex].w_count_ = wieght_count;
                frames_[*channel.nodeIndex].time_w_.reserve(input_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, input_acc,
                                              [&](float time) { frames_[*channel.nodeIndex].time_w_.push_back(time); });
                frames_[*channel.nodeIndex].w_.reserve(output_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, output_acc,
                                              [&](float w) { frames_[*channel.nodeIndex].w_.push_back(w); });
                break;
            }
        }
    }
}

void fi::res::gltf_frame::sample_kframe(float time_p)
{
}
