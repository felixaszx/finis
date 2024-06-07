#include <iostream>

#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include <ass.hpp>
#include <memory>

struct ModelLoader : public Extension, //
                     private VkObject
{
    TextureMgr textures_;
    inline static const std::string DEFAULT_TEXTURE = "res/textures/blank.png";
    inline static const std::string DEFAULT_DARK_TEXTURE = "res/textures/black.png";

    ModelLoader()
    {
        Texture t = textures_.load_texture(DEFAULT_TEXTURE);
        t = textures_.load_texture(DEFAULT_DARK_TEXTURE);
    }

    void load_model(MeshMemory& mesh_memory, std::string& name, const std::string& path,
                    uint32_t max_instances) override
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |    //
                                                           aiProcess_GenNormals | //
                                                           aiProcess_MakeLeftHanded);

        // Load vertices informations
        std::vector<std::byte> vert_idx_buffer;
        size_t total_vert_count = 0;
        size_t total_idx_count = 0;

        mesh_memory.meshes_.resize(scene->mNumMeshes, {});
        mesh_memory.materials_.resize(scene->mNumMaterials, {});
        mesh_memory.draw_calls_.resize(scene->mNumMeshes, {});
        for (uint32_t m = 0; m < scene->mNumMeshes; m++)
        {
            const aiMesh* mesh = scene->mMeshes[m];
            mesh_memory.meshes_[m].vertex_count_ = mesh->mNumVertices;
            mesh_memory.meshes_[m].draw_call_ = &mesh_memory.draw_calls_[m];
            mesh_memory.meshes_[m].material_ = &mesh_memory.materials_[mesh->mMaterialIndex];
            mesh_memory.draw_calls_[m].firstIndex = total_idx_count;
            mesh_memory.draw_calls_[m].vertexOffset = total_vert_count;
            mesh_memory.draw_calls_[m].indexCount = mesh->mNumFaces * 3;
            mesh_memory.draw_calls_[m].instanceCount = 1;
            mesh_memory.draw_calls_[m].firstInstance = 0;

            total_idx_count += mesh->mNumFaces * 3;
            total_vert_count += mesh->mNumVertices;
        }

        const size_t VEC3_SIZE = sizeof(float[3]);
        vert_idx_buffer.resize(total_vert_count * VEC3_SIZE * 3 + total_idx_count * sizeof(uint32_t));
        mesh_memory.position_offset_ = 0;
        mesh_memory.normal_offset_ = total_vert_count * VEC3_SIZE * 1;
        mesh_memory.tex_coord_offset_ = total_vert_count * VEC3_SIZE * 2;
        mesh_memory.idx_offset_ = total_vert_count * VEC3_SIZE * 3;

        mesh_memory.draw_call_offset_ = 0;
        mesh_memory.matrics_offset_ = mesh_memory.draw_calls_.size() * sizeof(mesh_memory.draw_calls_[0]);

        size_t vert_offset = 0;
        size_t idx_offset = 0;
        for (uint32_t m = 0; m < scene->mNumMeshes; m++)
        {
            const aiMesh* mesh = scene->mMeshes[m];
            memcpy(vert_idx_buffer.data() + mesh_memory.position_offset_ + vert_offset, //
                   mesh->mVertices,                                                     //
                   mesh->mNumVertices * VEC3_SIZE);
            memcpy(vert_idx_buffer.data() + mesh_memory.normal_offset_ + vert_offset, //
                   mesh->mNormals,                                                    //
                   mesh->mNumVertices * VEC3_SIZE);
            memcpy(vert_idx_buffer.data() + mesh_memory.tex_coord_offset_ + vert_offset, //
                   mesh->mTextureCoords[0],                                              //
                   mesh->mNumVertices * VEC3_SIZE);
            vert_offset += mesh->mNumVertices * VEC3_SIZE;

            for (uint32_t i = 0; i < mesh->mNumFaces; i++)
            {
                for (uint32_t ii = 0; ii < 3; ii++)
                {
                    auto idx = (float*)(vert_idx_buffer.data() + mesh_memory.idx_offset_ + idx_offset);
                    idx[ii] = mesh->mFaces[i].mIndices[ii];
                }
                idx_offset += 3 * sizeof(uint32_t);
            }
        }

        vk::BufferCreateInfo buffer_info{};
        buffer_info.size = vert_idx_buffer.size();
        buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
        vma::AllocationCreateInfo alloc_info{};
        alloc_info.usage = vma::MemoryUsage::eAutoPreferHost;
        alloc_info.preferredFlags = vk::MemoryPropertyFlagBits::eHostCoherent;
        alloc_info.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
        BufferAllocation staging = allocator().createBuffer(buffer_info, alloc_info);
        mesh_memory.vert_idx_buffer_ = std::make_unique<Buffer<Vertex, Index>>(vert_idx_buffer.size(), DST);

        void* mapping = allocator().mapMemory(staging);
        memcpy(mapping, vert_idx_buffer.data(), vert_idx_buffer.size());
        free_container_memory(vert_idx_buffer);
        allocator().unmapMemory(staging);

        vk::CommandBuffer cmd = one_time_cmd();
        begin_cmd(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        vk::BufferCopy region{};
        region.srcOffset = 0;
        region.dstOffset = 0;
        region.size = buffer_info.size;
        cmd.copyBuffer(staging, *mesh_memory.vert_idx_buffer_, region);
        cmd.end();
        submit_one_time_cmd(cmd);
        queues(GRAPHICS_QUEUE_IDX).waitIdle();
        allocator().destroyBuffer(staging, staging);

        // load textures
        std::string parent_path(path.begin(), path.begin() + path.rfind('/') + 1);
        auto texture_loader = [&](const std::string& file_name, Texture& texture)
        {
            if (file_name.length() > 0)
            {
                texture = textures_.load_texture(parent_path + file_name);
            }
            else
            {
                texture = textures_.load_texture(DEFAULT_DARK_TEXTURE);
            }
        };

        for (uint32_t mt = 0; mt < scene->mNumMaterials; mt++)
        {
            aiMaterial* material = scene->mMaterials[mt];

            aiString file;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &file);
            texture_loader(file.C_Str(), mesh_memory.materials_[mt].albedo_);

            file.Clear();
            material->GetTexture(aiTextureType_SPECULAR, 0, &file);
            texture_loader(file.C_Str(), mesh_memory.materials_[mt].specular_);

            file.Clear();
            material->GetTexture(aiTextureType_OPACITY, 0, &file);
            texture_loader(file.C_Str(), mesh_memory.materials_[mt].opacity_);

            file.Clear();
            material->GetTexture(aiTextureType_AMBIENT, 0, &file);
            texture_loader(file.C_Str(), mesh_memory.materials_[mt].ambient_);

            file.Clear();
            material->GetTexture(aiTextureType_NORMALS, 0, &file);
            texture_loader(file.C_Str(), mesh_memory.materials_[mt].normal_);

            file.Clear();
            material->GetTexture(aiTextureType_EMISSIVE, 0, &file);
            texture_loader(file.C_Str(), mesh_memory.materials_[mt].emissive_);
        }

        // setup instance matrices
        mesh_memory.matrices_.resize(max_instances, glm::mat4(1));
        vk::DeviceSize matrices_size = mesh_memory.matrices_.size() * sizeof(mesh_memory.matrices_[0]);
        vk::DeviceSize draw_call_size = mesh_memory.draw_calls_.size() * sizeof(mesh_memory.draw_calls_[0]);

        mesh_memory.matrices_bone_buffer_ = std::make_unique<Buffer<Uniform, Indirect>>(matrices_size + draw_call_size);
        mesh_memory.matrics_offset_ = 0;
        mesh_memory.draw_call_offset_ = matrices_size;
        memcpy(mesh_memory.matrices_bone_buffer_->map_memory(), //
               mesh_memory.matrices_.data(),                    //
               matrices_size);
        memcpy(mesh_memory.matrices_bone_buffer_->mapping() + matrices_size, //
               mesh_memory.draw_calls_.data(),                               //
               draw_call_size);

        // setup vertex input state
        vk::PipelineVertexInputStateCreateInfo& input_state = mesh_memory.vertex_input_state_;
        std::vector<vk::VertexInputBindingDescription> bindings(4);
        for (uint32_t b = 0; b < bindings.size(); b++)
        {
            bindings[b].binding = b;
            bindings[b].inputRate = vk::VertexInputRate::eVertex;
        }
        bindings[3].inputRate = vk::VertexInputRate::eInstance;
        std::vector<vk::VertexInputAttributeDescription> attributes(7);
        for (uint32_t i = 0; i < 3; i++)
        {
            attributes[i].binding = i;
            attributes[i].location = i;
            attributes[i].format = vk::Format::eR32G32B32Sfloat;
        }
        for (uint32_t i = 3; i < 7; i++)
        {
            attributes[i].binding = 3;
            attributes[i].location = i;
            attributes[i].format = vk::Format::eR32G32B32A32Sfloat;
            attributes[i].offset = (i - 3) * sizeof(glm::vec4);
        }
        input_state.setVertexBindingDescriptions(bindings);
        input_state.setVertexAttributeDescriptions(attributes);

        mesh_memory.binding_call_ = [&](vk::CommandBuffer cmd)
        {
            cmd.bindVertexBuffers(0, *mesh_memory.vert_idx_buffer_,
                                  {mesh_memory.position_offset_, //
                                   mesh_memory.normal_offset_,   //
                                   mesh_memory.tex_coord_offset_});
            cmd.bindIndexBuffer(*mesh_memory.vert_idx_buffer_, mesh_memory.idx_offset_, vk::IndexType::eUint32);
        };
    };
};

EXPORT_EXTENSION(ModelLoader);