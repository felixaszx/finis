#ifndef RESOURCES_GLTF_FILE_HPP
#define RESOURCES_GLTF_FILE_HPP

#include <format>

#include <stb/stb_image.h>

#include "tools.hpp"
#include "graphics/prim_data.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "bs_th_pool/BS_thread_pool.hpp"

namespace fi::res
{
    namespace fgltf = fastgltf;

    struct gltf_morph
    {
        std::string name_;
        std::vector<glm::vec3> positions_{};
        std::vector<glm::vec3> normals_{};
        std::vector<glm::vec4> tangents_{};
    };

    using gltf_mat = gfx::mat_info;

    struct gltf_tex
    {
        std::string name_;
        int x_;
        int y_;
        int comp_;
        std::vector<std::byte> data_{}; // decoded
    };

    struct gltf_prim
    {
        std::string name_;
        std::vector<glm::vec3> positions_{};
        std::vector<glm::vec3> normals_{};
        std::vector<glm::vec4> tangents_{};
        std::vector<glm::vec2> texcoords_{};
        std::vector<glm::uvec4> joints_{};
        std::vector<glm::vec4> weights_{};
        std::vector<uint32_t> idxs_{};

        uint32_t mesh_ = -1;
        uint32_t material_ = -1;
        uint32_t morph_ = -1;
    };

    struct gltf_mesh
    {
        std::string name_;
        std::vector<gltf_prim> prims_;
    };

    struct gltf_file
    {
        std::string name_;
        std::unique_ptr<fgltf::Asset> asset_;

        std::vector<gltf_mesh> meshes_{};
        std::vector<gltf_tex> textures_{};
        std::vector<gltf_morph> morphs_{};

        std::vector<std::string> mat_names_{};
        std::vector<gltf_mat> materials_{};

        gltf_file(const std::filesystem::path& path);
    };
}; // namespace fi::res

#endif // RESOURCES_GLTF_FILE_HPP