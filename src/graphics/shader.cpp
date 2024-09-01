#include "graphics/shader.hpp"
#include "slang-com-ptr.h"

struct SlangGlobalSession
{
    Slang::ComPtr<slang::IGlobalSession> session_;
    SlangGlobalSession() { slang::createGlobalSession(session_.writeRef()); }
    ~SlangGlobalSession() = default;
};

slang::IGlobalSession& fi::graphics::Shader::get_global_session()
{
    static SlangGlobalSession session;
    return *session.session_;
}

fi::graphics::Shader::Shader(const std::filesystem::path& shader_file, const std::filesystem::path& include_path)
{
    if (!std::filesystem::exists(shader_file))
    {
        throw std::runtime_error(std::format("{} do not exist", shader_file.generic_string()));
    }

    slang::TargetDesc target_desc;
    target_desc.forceGLSLScalarBufferLayout = true;
    target_desc.format = SLANG_SPIRV;
    target_desc.profile = get_global_session().findProfile("spirv_1_6");

    std::string search_path = include_path.generic_string();
    if (search_path.empty())
    {
        search_path = shader_file.parent_path().generic_string();
    }
    const char* const path_c = search_path.data();

    std::vector<slang::CompilerOptionEntry> options;
    options.emplace_back(slang::CompilerOptionName::GLSLForceScalarLayout,
                         slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, //
                                                    .intValue0 = 1});
    options.emplace_back(slang::CompilerOptionName::Capability,
                         slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, //
                                                    .intValue0 = get_global_session().findCapability("all")});
    options.emplace_back(
        slang::CompilerOptionName::Optimization,
        slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, //
                                   .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_HIGH});

    slang::SessionDesc session_desc{
        .targets = &target_desc,
        .targetCount = 1,
        .searchPaths = &path_c,
        .searchPathCount = 1,
        .compilerOptionEntries = options.data(),
        .compilerOptionEntryCount = casts(uint32_t, options.size()),
    };

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(get_global_session().createSession(session_desc, session.writeRef())))
    {
        throw std::runtime_error("Fail to create slang session");
    }

    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IModule> module(session->loadModule(shader_file.generic_string().c_str(), //
                                                             diagnostics.writeRef()));
    if (diagnostics)
    {
        std::cerr << castf(const char*, diagnostics->getBufferPointer());
        throw std::runtime_error("Fail to compile shader");
    }

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points_ref(module->getDefinedEntryPointCount());
    std::vector<slang::IComponentType*> components = {module};
    for (uint32_t i = 0; i < module->getDefinedEntryPointCount(); i++)
    {
        module->getDefinedEntryPoint(i, entry_points_ref[i].writeRef());
        components.push_back(entry_points_ref[i]);
    }

    Slang::ComPtr<slang::IComponentType> program;
    session->createCompositeComponentType(components.data(), components.size(), program.writeRef());

    Slang::ComPtr<slang::IComponentType> linked_program;
    Slang::ComPtr<ISlangBlob> diagnostic_blob;
    program->link(linked_program.writeRef(), diagnostic_blob.writeRef());

    Slang::ComPtr<slang::IBlob> kernel_blob;
    linked_program->getTargetCode(0, kernel_blob.writeRef(), diagnostics.writeRef());
    if (diagnostics)
    {
        std::cerr << castf(const char*, diagnostics->getBufferPointer());
    }

    vk::ShaderModuleCreateInfo shader_info{};
    shader_info.codeSize = kernel_blob->getBufferSize();
    shader_info.pCode = castr(const uint32_t*, kernel_blob->getBufferPointer());
    module_ = device().createShaderModule(shader_info);

    spvc::Compiler reflection(shader_info.pCode, shader_info.codeSize / sizeof(uint32_t));
    spvc::SmallVector<spvc::EntryPoint> entry_points = reflection.get_entry_points_and_stages();
    entrys_.reserve(entry_points.size());
    stage_infos_.reserve(entry_points.size());
    for (const auto& entry : entry_points)
    {
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
    }

    auto get_binding_info = [&](const spvc::Resource& res, vk::DescriptorType desc_type)
    {
        const spvc::SPIRType& type = reflection.get_type(res.type_id);
        uint32_t set = reflection.get_decoration(res.id, spv::DecorationDescriptorSet);
        desc_sets_.resize(set + 1);
        desc_names_.resize(set + 1);

        auto& binding = desc_sets_[set].emplace_back();
        desc_names_[set].push_back(res.name);
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

        if (desc_type == vk::DescriptorType::eSampledImage && type.image.dim == spv::DimBuffer)
        {
            binding.descriptorType = vk::DescriptorType::eUniformTexelBuffer;
        }
        else if (desc_type == vk::DescriptorType::eStorageImage)
        {
            binding.descriptorType = vk::DescriptorType::eStorageTexelBuffer;
        }
        return binding;
    };

    spvc::ShaderResources reses = reflection.get_shader_resources();
    for (const auto& res : reses.push_constant_buffers)
    {
        push_const_names_.push_back(res.name);
    }
    for (const auto& res : reses.sampled_images)
    {
        get_binding_info(res, vk::DescriptorType::eCombinedImageSampler);
    }
    for (const auto& res : reses.separate_images)
    {
        get_binding_info(res, vk::DescriptorType::eSampledImage);
    }
    for (const auto& res : reses.storage_images)
    {
        get_binding_info(res, vk::DescriptorType::eStorageImage);
    }
    for (const auto& res : reses.separate_samplers)
    {
        get_binding_info(res, vk::DescriptorType::eSampler);
    }
    for (const auto& res : reses.uniform_buffers)
    {
        get_binding_info(res, vk::DescriptorType::eUniformBuffer);
    }
    for (const auto& res : reses.subpass_inputs)
    {
        get_binding_info(res, vk::DescriptorType::eInputAttachment);
    }
    for (const auto& res : reses.storage_buffers)
    {
        get_binding_info(res, vk::DescriptorType::eStorageBuffer);
    }
}

fi::graphics::Shader::~Shader()
{
    device().destroyShaderModule(module_);
}
