#ifndef RESOURCES_GLTF_FILE_HPP
#define RESOURCES_GLTF_FILE_HPP

#include <format>

#include <stb/stb_image.h>

#include "tools.hpp"
#include "extensions/cpp_defines.hpp"
#include "graphics/prim_data.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/glm_element_traits.hpp"

namespace fi::res
{
    namespace fgltf = fastgltf;

    struct gltf_morph
    {
        std::vector<glm::vec3> positions_{};
        std::vector<glm::vec3> normals_{};
        std::vector<glm::vec3> tangents_{};

        uint32_t position_count_ = 0;
        uint32_t normal_count_ = 0;
        uint32_t tangent_count_ = 0;
    };

    using gltf_mat = gfx::mat_info;

    struct gltf_tex
    {
        std::string name_;
        int x_;
        int y_;
        int comp_;
        bool mipmapped_ = false;
        uint32_t sampler_idx_;
        std::vector<std::byte> data_{}; // decoded

        vk::Extent3D get_extent() { return {static_cast<uint32_t>(x_), static_cast<uint32_t>(y_), 1}; };
        uint32_t get_levels() { return mipmapped_ ? std::floor(std::log2(std::max(x_, y_))) + 1 : 1; }
    };

    struct gltf_prim
    {
        std::string name_;
        std::vector<glm::vec3> positions_{};
        std::vector<glm::vec3> normals_{};
        std::vector<glm::vec4> tangents_{};
        std::vector<glm::vec2> texcoords_{};
        std::vector<glm::vec4> colors_{};
        std::vector<glm::uvec4> joints_{};
        std::vector<glm::vec4> weights_{};
        std::vector<uint32_t> idxs_{};

        uint32_t mesh_ = -1;
        uint32_t material_ = -1;
        gltf_morph morph_;
    };

    struct gltf_mesh
    {
        std::string name_;
        std::vector<gltf_prim> prims_;
        std::vector<vk::DrawIndirectCommand> draw_calls_{};
    };

    struct gltf_file
    {
      private:
        size_t prim_count_ = 0;

      public:
        std::string name_;
        std::unique_ptr<fgltf::Asset> asset_;

        std::vector<gltf_mesh> meshes_{};
        std::vector<gltf_tex> textures_{};
        std::vector<vk::SamplerCreateInfo> samplers_{};

        std::vector<std::string> mat_names_{};
        std::vector<gltf_mat> materials_{};

        gltf_file(const std::filesystem::path& path,
                  std::vector<std::future<void>>* futs,
                  thp::task_thread_pool* th_pool = nullptr);
        ~gltf_file() = default;

        [[nodiscard]] size_t prim_count() const { return prim_count_; }
    };
}; // namespace fi::res

#endif // RESOURCES_GLTF_FILE_HPP