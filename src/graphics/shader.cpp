/**
 * @file shader.cpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2024-08-15
 * 
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 * 
 */
#include "graphics/shader.hpp"

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

    auto kind_func = [&]()
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
                return shaderc_vertex_shader;
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