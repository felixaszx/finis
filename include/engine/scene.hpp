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
        std::vector<ResDetails> res_details_;
        std::vector<ResStructure> res_structures_;
        std::vector<ResSkinDetails> res_skins_;
        std::vector<std::vector<ResAnimation>> res_anims_;

        std::filesystem::path res_path_ = "";

        SceneResources(const std::filesystem::path& res_path);
    };
}; // namespace fi

#endif // ENGINE_SCENE_HPP
