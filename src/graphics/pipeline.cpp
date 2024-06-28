#include "graphics/pipeline.hpp"
#include "tools.hpp"

fi::ShaderModule::ShaderModule(const std::string& file_name, //
                               vk::ShaderStageFlagBits stage)
    : file_name_(file_name)
{
    reset(file_name, stage);
}

fi::ShaderModule::~ShaderModule()
{
    device().destroyShaderModule(*this);
}

void fi::ShaderModule::reset(const std::string& file_name, //
                             vk::ShaderStageFlagBits stage)
{
    file_name_ = file_name;
    this->stage = stage;
    reset();
}

void fi::ShaderModule::reset()
{
    if (static_cast<vk::ShaderModule>(*this))
    {
        device().destroyShaderModule(*this);
    }

    std::ifstream file(file_name_, std::ios::in);
    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file\n");
    }

    std::stringstream sstr;
    sstr << file.rdbuf();
    std::string code = sstr.str();

    auto kind_func = [&]() -> shaderc_shader_kind
    {
        switch (stage)
        {
            case vk::ShaderStageFlagBits::eVertex:
                return shaderc_vertex_shader;
            case vk::ShaderStageFlagBits::eFragment:
                return shaderc_fragment_shader;
            case vk::ShaderStageFlagBits::eGeometry:
                return shaderc_geometry_shader;
            case vk::ShaderStageFlagBits::eCompute:
                return shaderc_compute_shader;
            default:
                return shaderc_mesh_shader;
        }
    };

    auto result = compiler_.CompileGlslToSpv(code, kind_func(), file_name_.c_str());
    std::vector<uint32_t> binary(result.begin(), result.end());

    if (!result.GetErrorMessage().empty())
    {
        for (auto& msg : result.GetErrorMessage())
        {
            std::cerr << msg;
        }
        return;
    }

    vk::ShaderModuleCreateInfo create_info{};
    create_info.setCode(binary);
    static_cast<vk::ShaderModule&>(*this) = device().createShaderModule(create_info);
    this->module = *this;
    this->pName = "main";
}

fi::CombinedPipeline fi::PipelineMgr::build_pipeline(size_t layout_idx, const vk::GraphicsPipelineCreateInfo& info)
{
    CombinedPipeline pipeline{};
    pipeline.type_ = vk::PipelineBindPoint::eGraphics;
    sset(pipeline, device().createGraphicsPipelines(pipeline_cache(), info).value[0], layouts_[layout_idx]);
    pipelines_.push_back(pipeline);
    return pipeline;
}

fi::CombinedPipeline fi::PipelineMgr::build_pipeline(size_t layout_idx, const vk::ComputePipelineCreateInfo& info)
{
    CombinedPipeline pipeline{};
    pipeline.type_ = vk::PipelineBindPoint::eCompute;
    sset(pipeline, device().createComputePipelines(pipeline_cache(), info).value[0], layouts_[layout_idx]);
    pipelines_.push_back(pipeline);
    return pipeline;
}

void fi::PipelineMgr::bind_pipeline(vk::CommandBuffer cmd, const CombinedPipeline& pipeline)
{
    if (prev_ != pipeline)
    {
        cmd.bindPipeline(pipeline.type_, pipeline);
        prev_ = pipeline;
    }
}

void fi::PipelineMgr::bind_descriptor_sets(vk::CommandBuffer cmd, const vk::ArrayProxy<const vk::DescriptorSet>& sets,
                                           uint32_t first_set, const vk::ArrayProxy<const uint32_t>& dynamic_offsets)
{
    cmd.bindDescriptorSets(prev_.type_, prev_, first_set, sets, dynamic_offsets);
}

size_t fi::PipelineMgr::build_pipeline_layout(const vk::PipelineLayoutCreateInfo& info)
{
    layouts_.push_back(device().createPipelineLayout(info));
    return layouts_.size() - 1;
}
