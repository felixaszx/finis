#ifndef GRAPHICS_PRIM_RES_HPP
#define GRAPHICS_PRIM_RES_HPP

#include "graphics/prims.hpp"
#include "graphics/textures.hpp"
#include "extensions/cpp_defines.hpp"

namespace fi::gfx
{
    struct pipeline_pkg
    {
        gfx::primitives* prims_ = nullptr;
        gfx::prim_structure* structs_ = nullptr;
        gfx::prim_skins* skins_ = nullptr;
        gfx::tex_arr* tex_arr_ = nullptr;
    };

    struct prim_res : protected graphcis_obj
    {
        std::string name_ = "";
        virtual gfx::primitives* get_primitives() = 0;
        virtual gfx::prim_structure* get_prim_structure() = 0;
        virtual gfx::prim_skins* get_prim_skin() = 0;
        virtual gfx::tex_arr* get_tex_arr() = 0;

        virtual ~prim_res() = default;

        gfx::pipeline_pkg get_pipeline_pkg()
        {
            return {get_primitives(), get_prim_structure(), get_prim_skin(), get_tex_arr()};
        }
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PRIM_RES_HPP