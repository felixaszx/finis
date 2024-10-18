#ifndef DEFFER_META_H
#define DEFFER_META_H

#include <stb/stb_ds.h>

#include "fi_ext.h"
#include "gfx/gfx.h"

typedef enum deffer_renderer_atchm_usage
{
    DEFFER_RENDERER_POSITION_ATCHM,
    DEFFER_RENDERER_COLOR_ATCHM,
    DEFFER_RENDERER_NORMAL_ATCHM,
    DEFFER_RENDERER_SPEC_ATCHM,
    DEFFER_RENDERER_DEPTH_STENCIL_ATCHM,
    DEFFER_RENDERER_LIGHT_ATCHM
} deffer_renderer_atchm_usage;

typedef enum deffer_renderer_state
{
    HOLDER,
} deffer_renderer_state;

typedef struct mesh_pkg
{
    vk_mesh* mesh_;
    vk_mesh_desc* mesh_desc_;
    vk_mesh_skin* mesh_skin_;
    vk_tex_arr* tex_arr_;
} mesh_pkg;

typedef struct render_pkg
{
    mesh_pkg* mesh_pkgs_; // stb_ds::arr
} render_pkg;

DEFINE_OBJ_DEFAULT(render_pkg);
size_t render_pkg_get_max_mesh_pkgs(render_pkg* this);
mesh_pkg* render_pkg_add_mesh_pkg(render_pkg* this);
void render_pkg_reset(render_pkg* this);

#endif // DEFFER_META_H