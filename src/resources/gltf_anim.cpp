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
                fgltf::iterateAccessor<float>(*file.asset_, input_acc, [&](float time)
                                              { frames_[*channel.nodeIndex].time_t_.push_back(time * 1000); });
                frames_[*channel.nodeIndex].t_.reserve(output_acc.count);
                fgltf::iterateAccessor<glm::vec3>(*file.asset_, output_acc, [&](const glm::vec3& t)
                                                  { frames_[*channel.nodeIndex].t_.push_back(t); });
                break;
            }
            case fgltf::AnimationPath::Rotation:
            {
                frames_[*channel.nodeIndex].time_r_.reserve(input_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, input_acc, [&](float time)
                                              { frames_[*channel.nodeIndex].time_r_.push_back(time * 1000); });
                frames_[*channel.nodeIndex].r_.reserve(output_acc.count);
                fgltf::iterateAccessor<fgltf::math::quat<float>>(
                    *file.asset_, output_acc, [&](const fgltf::math::quat<float>& r)
                    { glms::assign_value(frames_[*channel.nodeIndex].r_.emplace_back(), r); });
                break;
            }
            case fgltf::AnimationPath::Scale:
            {
                frames_[*channel.nodeIndex].time_s_.reserve(input_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, input_acc, [&](float time)
                                              { frames_[*channel.nodeIndex].time_s_.push_back(time * 1000); });
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
                fgltf::iterateAccessor<float>(*file.asset_, input_acc, [&](float time)
                                              { frames_[*channel.nodeIndex].time_w_.push_back(time * 1000); });
                frames_[*channel.nodeIndex].w_.reserve(output_acc.count);
                fgltf::iterateAccessor<float>(*file.asset_, output_acc,
                                              [&](float w) { frames_[*channel.nodeIndex].w_.push_back(w); });
                break;
            }
        }
    }
}

void fi::res::gltf_frame::sample_kframe(size_t time_p)
{
    if (t_.size() > 0)
    {
        size_t time = time_t_.front() + time_p % (time_t_.back() - time_t_.front());
        auto upper_bound = std::upper_bound(time_t_.begin(), time_t_.end(), time);
        auto base = upper_bound - 1;
        size_t sample = base - time_t_.begin();
        glm::vec3 translation = glm::lerp(t_[sample], t_[sample + 1], (time - *base) / float(*upper_bound - *base));
        *t_out_ = glm::translate(translation);
    }

    if (r_.size() > 0)
    {
        size_t time = time_r_.front() + time_p % (time_r_.back() - time_r_.front());
        auto upper_bound = std::upper_bound(time_r_.begin(), time_r_.end(), time);
        auto base = upper_bound - 1;
        size_t sample = base - time_r_.begin();
        glm::quat rotation = glm::slerp(r_[sample], r_[sample + 1], (time - *base) / float(*upper_bound - *base));
        *r_out_ = glm::mat4(rotation);
    }

    if (s_.size() > 0)
    {
        size_t time = time_s_.front() + time_p % (time_s_.back() - time_s_.front());
        auto upper_bound = std::upper_bound(time_s_.begin(), time_s_.end(), time);
        auto base = upper_bound - 1;
        size_t sample = base - time_s_.begin();
        glm::vec3 scale = glm::lerp(s_[sample], s_[sample + 1], (time - *base) / float(*upper_bound - *base));
        *s_out_ = glm::scale(scale);
    }

    if (w_.size() > 0)
    {
        size_t time = time_w_.front() + time_p % (time_w_.back() - time_w_.front());
        auto upper_bound = std::upper_bound(time_w_.begin(), time_w_.end(), time);
        auto base = upper_bound - 1;
        size_t sample = w_count_ * (base - time_w_.begin());
        size_t sample_next = sample + w_count_;
        for (size_t i = 0; i < w_count_; i++)
        {
            w_out_[i] = glm::lerp(w_[sample + i], w_[sample_next + i], (time - *base) / float(*upper_bound - *base));
        }
    }
}