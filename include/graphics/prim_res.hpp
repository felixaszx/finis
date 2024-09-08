#ifndef GRAPHICS_PRIM_RES_HPP
#define GRAPHICS_PRIM_RES_HPP

#include "graphics/prims.hpp"
#include "graphics/textures.hpp"
#include "extensions/cpp_defines.hpp"

namespace fi::gfx
{
    struct prim_res : public ext::base, protected graphcis_obj
    {
        virtual gfx::primitives* get_primitives() = 0;
        virtual gfx::prim_structure* get_prim_structure() = 0;
        virtual gfx::prim_skins* get_prim_skin() = 0;
        virtual gfx::tex_arr* get_tex_arr() = 0;
    };
}; // namespace fi::gfx

#endif // GRAPHICS_PRIM_RES_HPP