#include "graphics/shader.hpp"
#include "slang-com-ptr.h"

fi::gfx::shader::shader(const std::filesystem::path& shader_file, const std::filesystem::path& include_path)
{
    if (!std::filesystem::exists(shader_file))
    {
        throw std::runtime_error(std::format("{} do not exist", shader_file.generic_string()));
    }

    std::ifstream file(shader_file, std::ifstream::ate | std::ios::binary);
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    if (!file.read(buffer.data(), buffer.size()))
    {
        file.close();
        throw std::runtime_error(std::format("fail to read {}", shader_file.generic_string()));
    }
    file.close();

    vk::ShaderModuleCreateInfo shader_info{};
    shader_info.codeSize = buffer.size();
    shader_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    module_ = device().createShaderModule(shader_info);

    spvc::Compiler reflection(shader_info.pCode, shader_info.codeSize / sizeof(uint32_t));
    spvc::SmallVector<spvc::EntryPoint> entry_points = reflection.get_entry_points_and_stages();
    entrys_.reserve(entry_points.size());
    stage_infos_.reserve(entry_points.size());
    for (const auto& entry : entry_points)
    {
        reflection.set_entry_point(entry.name, entry.execution_model);
        vk::PipelineShaderStageCreateInfo& stage_info = stage_infos_.emplace_back();
        switch (entry.execution_model)
        {
            case spv::ExecutionModelVertex:
                stage_info.stage = vk::ShaderStageFlagBits::eVertex;
                break;
            case spv::ExecutionModelTessellationControl:
                stage_info.stage = vk::ShaderStageFlagBits::eTessellationControl;
                break;
            case spv::ExecutionModelTessellationEvaluation:
                stage_info.stage = vk::ShaderStageFlagBits::eTessellationEvaluation;
                break;
            case spv::ExecutionModelGeometry:
                stage_info.stage = vk::ShaderStageFlagBits::eGeometry;
                break;
            case spv::ExecutionModelFragment:
                stage_info.stage = vk::ShaderStageFlagBits::eFragment;
                break;
            case spv::ExecutionModelGLCompute:
                stage_info.stage = vk::ShaderStageFlagBits::eCompute;
                break;
            case spv::ExecutionModelTaskNV:
                stage_info.stage = vk::ShaderStageFlagBits::eTaskNV;
                break;
            case spv::ExecutionModelMeshNV:
                stage_info.stage = vk::ShaderStageFlagBits::eMeshNV;
                break;
            case spv::ExecutionModelRayGenerationKHR:
                stage_info.stage = vk::ShaderStageFlagBits::eRaygenKHR;
                break;
            case spv::ExecutionModelIntersectionKHR:
                stage_info.stage = vk::ShaderStageFlagBits::eIntersectionKHR;
                break;
            case spv::ExecutionModelAnyHitKHR:
                stage_info.stage = vk::ShaderStageFlagBits::eAnyHitKHR;
                break;
            case spv::ExecutionModelClosestHitKHR:
                stage_info.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
                break;
            case spv::ExecutionModelMissKHR:
                stage_info.stage = vk::ShaderStageFlagBits::eMissKHR;
                break;
            case spv::ExecutionModelCallableKHR:
                stage_info.stage = vk::ShaderStageFlagBits::eCallableKHR;
                break;
            case spv::ExecutionModelTaskEXT:
                stage_info.stage = vk::ShaderStageFlagBits::eTaskEXT;
                break;
            case spv::ExecutionModelMeshEXT:
                stage_info.stage = vk::ShaderStageFlagBits::eMeshEXT;
                break;
            case spv::ExecutionModelKernel:
            case spv::ExecutionModelMax:
                stage_info.stage = vk::ShaderStageFlagBits::eAll;
                break;
        }
        entrys_.push_back(entry.name);
        stage_info.pName = entrys_.back().c_str();
        stage_info.module = module_;

        // return the set index and descriptor count
        auto get_binding_info = [&](const spvc::Resource& res, vk::DescriptorType desc_type)
        {
            const spvc::SPIRType& type = reflection.get_type(res.type_id);
            uint32_t set = reflection.get_decoration(res.id, spv::DecorationDescriptorSet);

            if (set >= desc_sets_.size())
            {
                desc_sets_.resize(set + 1);
                desc_names_.resize(set + 1);
            }

            auto& binding = desc_sets_[set].emplace_back();
            desc_names_[set].emplace_back(res.name, -1);
            binding.binding = reflection.get_decoration(res.id, spv::DecorationBinding);
            binding.stageFlags = vk::ShaderStageFlagBits::eAll;
            binding.descriptorType = desc_type;
            binding.descriptorCount = type.array.size() ? -1 : 1;

            bool fixed_arr = true;
            for (bool literal : type.array_size_literal)
            {
                fixed_arr = literal && fixed_arr;
            }

            if (fixed_arr)
            {
                for (uint32_t arr_size : type.array)
                {
                    binding.descriptorCount += arr_size;
                }
            }

            return std::pair(set, binding.descriptorCount);
        };

        spvc::ShaderResources reses = reflection.get_shader_resources();

        push_consts_.reserve(reses.push_constant_buffers.size());
        push_stages_.reserve(reses.push_constant_buffers.size());
        for (const auto& res : reses.push_constant_buffers)
        {
            const spvc::SPIRType& type = reflection.get_type(res.base_type_id);
            push_consts_.emplace_back(res.name, reflection.get_declared_struct_size(type));
            push_stages_.push_back(stage_info.stage);
        }
        for (const auto& res : reses.sampled_images)
        {
            auto p = get_binding_info(res, vk::DescriptorType::eCombinedImageSampler);
            if (p.second != -1)
            {
                desc_names_[p.first].back().second = image_infos_.size();
                image_infos_.resize(image_infos_.size() + p.second,
                                    vk::DescriptorImageInfo{.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal});
            }
        }
        for (const auto& res : reses.storage_images)
        {
            auto p = get_binding_info(res, vk::DescriptorType::eStorageImage);
            if (p.second != -1)
            {
                desc_names_[p.first].back().second = image_infos_.size();
                image_infos_.resize(image_infos_.size() + p.second,
                                    vk::DescriptorImageInfo{.imageLayout = vk::ImageLayout::eGeneral});
            }
        }
        for (const auto& res : reses.uniform_buffers)
        {
            auto p = get_binding_info(res, vk::DescriptorType::eUniformBuffer);
            if (p.second != -1)
            {
                const spvc::SPIRType& type = reflection.get_type(res.type_id);
                size_t size = reflection.get_declared_struct_size(type);
                desc_names_[p.first].back().second = buffer_infos_.size();
                buffer_infos_.resize(buffer_infos_.size() + p.second, vk::DescriptorBufferInfo{.range = size});
            }
        }
        for (const auto& res : reses.storage_buffers)
        {
            auto p = get_binding_info(res, vk::DescriptorType::eStorageBuffer);
            if (p.second != -1)
            {
                const spvc::SPIRType& type = reflection.get_type(res.type_id);
                size_t size = reflection.get_declared_struct_size(type);
                desc_names_[p.first].back().second = buffer_infos_.size();
                buffer_infos_.resize(buffer_infos_.size() + p.second, vk::DescriptorBufferInfo{.range = size});
            }
        }

        if (stage_info.stage == vk::ShaderStageFlagBits::eFragment)
        {
            atchm_count_ = reses.stage_outputs.size();
        }
    }
}

fi::gfx::shader::~shader()
{
    device().destroyShaderModule(module_);
}