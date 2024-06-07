#ifndef GRAPHICS_MESH_HPP
#define GRAPHICS_MESH_HPP

#include "buffer.hpp"
#include "texture.hpp"
#include "glms.hpp"
#include "extensions/loader.hpp"

struct Material
{
    // textures
    Texture albedo_{};
    Texture specular_{};
    Texture opacity_{};
    Texture ambient_{};
    Texture normal_{};
    Texture emissive_{};
};

struct Mesh
{
    vk::DeviceSize bone_id_offset_ = 0;     // tbd
    vk::DeviceSize bone_weight_offset_ = 0; // tbd
    uint32_t vertex_count_ = 0;

    vk::DrawIndexedIndirectCommand* draw_call_ = nullptr;
    Material* material_ = nullptr;
};

struct MeshMemory
{
    vk::DeviceSize position_offset_ = 0;
    vk::DeviceSize normal_offset_ = 0;
    vk::DeviceSize tex_coord_offset_ = 0;
    vk::DeviceSize idx_offset_ = 0;
    vk::DeviceSize bone_id_offset_ = 0;     // tbd
    vk::DeviceSize bone_weight_offset_ = 0; // tbd
    std::vector<Mesh> meshes_{};
    std::vector<Material> materials_{};
    std::unique_ptr<Buffer<Vertex, Index>> vert_idx_buffer_{};

    vk::DeviceSize draw_call_offset_ = 0;
    vk::DeviceSize matrics_offset_ = 0;
    vk::DeviceSize bones_offset_ = 0; // tbd
    std::vector<glm::mat4> matrices_{};
    std::vector<vk::DrawIndexedIndirectCommand> draw_calls_{};
    std::unique_ptr<Buffer<Uniform, Indirect>> matrices_bone_buffer_{};

    vk::PipelineVertexInputStateCreateInfo vertex_input_state_{};
    std::function<void(vk::CommandBuffer cmd)> binding_call_ = [](vk::CommandBuffer cmd) {};
};

class Model
{
  private:
    std::string name_ = "";
    MeshMemory mesh_memory_{};

  public:
    Model(const std::string& path, std::unique_ptr<Extension>& loading_ext, uint32_t max_instances = 10);
    ~Model();

    Mesh get_mesh(uint32_t id);
    void modify_instance_matric(uint32_t instance, const glm::mat4& matrics);
    void bind_vert_idx_buffer(vk::CommandBuffer cmd);
};

#endif // GRAPHICS_MESH_HPP
