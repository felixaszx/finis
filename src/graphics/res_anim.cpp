#include "graphics/res_anim.hpp"

std::vector<fi::ResAnimation> fi::get_res_animations(ResDetails& res_details, ResStructure& res_structure,
                                                     size_t gltf_idx)
{
    if (res_details.locked())
    {
        return {};
    }
    const fgltf::Asset& gltf = res_details.gltf_[gltf_idx].get();

    std::vector<ResAnimation> animations;
    animations.reserve(gltf.animations.size());
    for (const fgltf::Animation& anim_in : gltf.animations)
    {
        ResAnimation& anim = animations.emplace_back();
        anim.name_ = anim_in.name;

        const auto& channels = anim_in.channels;
        const auto& samplers = anim_in.samplers;
        for (const fgltf::AnimationChannel& channel : channels)
        {
            if (!channel.nodeIndex)
            {
                continue;
            }

            auto find_node_channel = [&](const CombinedAnimChannel& channel_in)
            { return channel_in.node_idx_ == channel.nodeIndex.value(); };
            auto target = std::find_if(anim.channels_.begin(), anim.channels_.end(), find_node_channel);
            if (target == anim.channels_.end())
            {
                anim.channels_.emplace_back();
                target = anim.channels_.end() - 1;
                target->node_idx_ = TSNodeIdx(channel.nodeIndex.value());
                target->node_ref_ = &res_structure.index_node(target->node_idx_, gltf_idx);
            }

            if (channel.path == fgltf::AnimationPath::Weights //
                && !target->weights_                          //
                && gltf.nodes[channel.nodeIndex.value()].meshIndex)
            {
                TSMeshIdx mesh_idx(res_details.first_mesh_[gltf_idx] //
                                   + gltf.nodes[channel.nodeIndex.value()].meshIndex.value());
                target->weights_ = &res_structure.morph_weight_[res_details.meshes_[mesh_idx].morph_weight_];
                target->weights_count_ = gltf.meshes     //
                                             [gltf.nodes //
                                                  [channel.nodeIndex.value()]
                                                      .meshIndex.value()]
                                                 .weights.size();
            }

            const fgltf::AnimationSampler& sampler = samplers[channel.samplerIndex];
            const fgltf::Accessor& timeline_acc = gltf.accessors[sampler.inputAccessor];
            const fgltf::Accessor& sample_acc = gltf.accessors[sampler.outputAccessor];
            switch (channel.path)
            {
                case fgltf::AnimationPath::Translation:
                {
                    target->translation_timeline_.reserve(timeline_acc.count);
                    target->translation_samples_.reserve(sample_acc.count);
                    fgltf::iterateAccessor<float>( //
                        gltf, timeline_acc,        //
                        [&](float time_point) { target->translation_timeline_.push_back(time_point); });
                    fgltf::iterateAccessor<fgltf::math::fvec3>(
                        gltf, sample_acc, //
                        [&](const fgltf::math::fvec3& sample)
                        { glms::assign_value(target->translation_samples_.emplace_back(), sample); });
                    break;
                }
                case fgltf::AnimationPath::Rotation:
                {
                    target->rotation_timeline_.reserve(timeline_acc.count);
                    target->rotation_samples_.reserve(sample_acc.count);
                    fgltf::iterateAccessor<float>( //
                        gltf, timeline_acc,        //
                        [&](float time_point) { target->rotation_timeline_.push_back(time_point); });
                    fgltf::iterateAccessor<fgltf::math::quat<float>>(
                        gltf, sample_acc, //
                        [&](const fgltf::math::quat<float>& sample)
                        { glms::assign_value(target->rotation_samples_.emplace_back(), sample); });
                    break;
                }
                case fgltf::AnimationPath::Scale:
                {
                    target->scale_timeline_.reserve(timeline_acc.count);
                    target->scale_samples_.reserve(sample_acc.count);
                    fgltf::iterateAccessor<float>( //
                        gltf, timeline_acc,        //
                        [&](float time_point) { target->scale_timeline_.push_back(time_point); });
                    fgltf::iterateAccessor<fgltf::math::fvec3>(
                        gltf, sample_acc, //
                        [&](const fgltf::math::fvec3& sample)
                        { glms::assign_value(target->scale_samples_.emplace_back(), sample); });
                    break;
                }
                case fgltf::AnimationPath::Weights:
                {
                    target->weight_timeline_.reserve(timeline_acc.count);
                    target->weight_samples_.reserve(sample_acc.count);
                    fgltf::iterateAccessor<float>( //
                        gltf, timeline_acc,        //
                        [&](float time_point) { target->weight_timeline_.push_back(time_point); });
                    fgltf::iterateAccessor<float>( //
                        gltf, sample_acc,          //
                        [&](float sample) { target->weight_samples_.push_back(sample); });
                    break;
                }
            }
        }
    }
    return animations;
}

void fi::CombinedAnimChannel::sample_animation(float time_point)
{
    if (!translation_timeline_.empty())
    {

        if (time_point == translation_timeline_.back())
        {
            node_ref_->translation_ = translation_samples_.back();
        }
        else
        {
            float translation_time_point = std::fmod(time_point, translation_timeline_.back());
            auto upper_time = std::upper_bound(translation_timeline_.begin(), //
                                               translation_timeline_.end(),   //
                                               translation_time_point);
            size_t upper_idx = upper_time - translation_timeline_.begin();
            float dt = translation_time_point - *(upper_time - 1);
            float t = *upper_time - *(upper_time - 1);
            float l = dt / t;
            node_ref_->translation_ = glm::lerp(translation_samples_[upper_idx - 1], //
                                                translation_samples_[upper_idx],     //
                                                l);
        }
    }

    if (!rotation_timeline_.empty())
    {
        if (time_point == rotation_timeline_.back())
        {
            node_ref_->rotation_ = rotation_samples_.back();
        }
        else
        {
            float rotation_time_point = std::fmod(time_point, rotation_timeline_.back());
            auto upper_time = std::upper_bound(rotation_timeline_.begin(), //
                                               rotation_timeline_.end(),   //
                                               rotation_time_point);
            size_t upper_idx = upper_time - rotation_timeline_.begin();
            float dt = rotation_time_point - *(upper_time - 1);
            float t = *upper_time - *(upper_time - 1);
            float l = dt / t;
            node_ref_->rotation_ = glm::slerp(rotation_samples_[upper_idx - 1], //
                                              rotation_samples_[upper_idx],     //
                                              l);
        }
    }

    if (!scale_timeline_.empty())
    {
        if (time_point == scale_timeline_.back())
        {
            node_ref_->scale_ = scale_samples_.back();
        }
        else
        {
            float scale_time_point = std::fmod(time_point, scale_timeline_.back());
            auto upper_time = std::upper_bound(scale_timeline_.begin(), //
                                               scale_timeline_.end(),   //
                                               scale_time_point);
            size_t upper_idx = upper_time - scale_timeline_.begin();
            float dt = scale_time_point - *(upper_time - 1);
            float t = *upper_time - *(upper_time - 1);
            float l = dt / t;
            node_ref_->scale_ = glm::lerp(scale_samples_[upper_idx - 1], //
                                          scale_samples_[upper_idx],     //
                                          l);
        }
    }

    if (!weight_timeline_.empty())
    {
        if (time_point == weight_timeline_.back())
        {
            size_t idx = weight_samples_.size() - weights_count_;
            for (uint32_t i = 0; i < weights_count_; i++)
            {
                weights_[i] = weight_samples_[idx + i];
            }
        }
        else
        {
            float weight_time_point = fmod(time_point, weight_timeline_.back());
            auto upper_time = std::upper_bound(weight_timeline_.begin(), //
                                               weight_timeline_.end(),   //
                                               weight_time_point);
            size_t upper_idx = upper_time - weight_timeline_.begin();
            float dt = weight_time_point - *(upper_time - 1);
            float t = *upper_time - *(upper_time - 1);
            float l = dt / t;

            size_t prev_idx = (upper_idx - 1) * weights_count_;
            size_t next_idx = upper_idx * weights_count_;
            for (uint32_t i = 0; i < weights_count_; i++)
            {
                weights_[i] = weight_samples_[next_idx + i] - weight_samples_[prev_idx + i];
                weights_[i] = weight_samples_[prev_idx + i] + l * weights_[i];
            }
        }
    }
}
void fi::ResAnimation::set_keyframe(float time_point)
{
    for (fi::CombinedAnimChannel& channel : channels_)
    {
        channel.sample_animation(time_point);
    }
}
