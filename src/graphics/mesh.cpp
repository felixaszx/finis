#include "graphics/mesh.hpp"

Model::Model(const std::string& path, std::unique_ptr<Extension>& loading_ext, uint32_t max_instances)
{
    loading_ext->load_model(mesh_memory_, name_, path, max_instances);
}

Model::~Model()
{
}

Mesh Model::get_mesh(uint32_t id)
{
    return mesh_memory_.meshes_[id];
}

void Model::modify_instance_matric(uint32_t instance, const glm::mat4& matrics)
{
    mesh_memory_.matrices_[instance] = matrics;
}

void Model::bind_vert_idx_buffer(vk::CommandBuffer cmd)
{
    mesh_memory_.binding_call_(cmd);
}
