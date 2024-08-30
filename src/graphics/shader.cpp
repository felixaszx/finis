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
    slang::TargetDesc target_desc;
    target_desc.forceGLSLScalarBufferLayout = true;
    target_desc.format = SLANG_SPIRV;
    target_desc.profile = get_global_session().findProfile("glsl_450");

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
    options.emplace_back(slang::CompilerOptionName::EntryPointName,
                         slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::String, //
                                                    .stringValue0 = ENTRY_POINT_});

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
    }

    Slang::ComPtr<slang::IEntryPoint> entry_point;
    module->findEntryPointByName(ENTRY_POINT_, entry_point.writeRef());

    std::array<slang::IComponentType*, 2> components = {module, entry_point};
    Slang::ComPtr<slang::IComponentType> program;
    session->createCompositeComponentType(components.data(), 2, program.writeRef());

    Slang::ComPtr<slang::IComponentType> linked_program;
    Slang::ComPtr<ISlangBlob> diagnostic_blob;
    program->link(linked_program.writeRef(), diagnostic_blob.writeRef());

    Slang::ComPtr<slang::IBlob> kernel_blob;
    linked_program->getEntryPointCode(0, 0, kernel_blob.writeRef(), diagnostics.writeRef());
    if (diagnostics)
    {
        std::cerr << castf(const char*, diagnostics->getBufferPointer());
    }

    vk::ShaderModuleCreateInfo shader_info{.codeSize = kernel_blob->getBufferSize(),
                                           .pCode = castr(const uint32_t*, kernel_blob->getBufferPointer())};
    shader_ = device().createShaderModule(shader_info);
    stage_info_.module = shader_;
    stage_info_.pName = ENTRY_POINT_;

    slang::ProgramLayout* layout = program->getLayout(0, diagnostics.writeRef());
    if (diagnostics)
    {
        std::cerr << castf(const char*, diagnostics->getBufferPointer());
    }
}

fi::graphics::Shader::~Shader()
{
}
