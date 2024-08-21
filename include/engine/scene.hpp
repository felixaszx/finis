#ifndef ENGINE_SCENE_HPP
#define ENGINE_SCENE_HPP

#include "graphics/res_loader.hpp"
#include "graphics/res_structure.hpp"
#include "graphics/res_anim.hpp"
#include "graphics/res_skin.hpp"

namespace fi
{
    struct SceneResources
    {
        struct InstancingInfo
        {
            uint32_t first_matrix_ = EMPTY;
            uint32_t matrix_count_ = 0;
        };

        UniqueObj<ResDetails> res_detail_;
        UniqueObj<ResStructure> res_structure_{nullptr};
        UniqueObj<ResSkinDetails> res_skin_{nullptr};
        std::vector<InstancingInfo> instancing_infos_{};
        std::vector<std::vector<ResAnimation>> res_anims_{};
        std::vector<glm::mat4> instancing_matrices_{};

        std::filesystem::path res_path_ = "";

        SceneResources(const std::filesystem::path& res_path, uint32_t max_instances = 0);
    };
}; // namespace fi

#endif // ENGINE_SCENE_HPP
