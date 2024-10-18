#include "meta.h"

IMPL_OBJ_NEW_DEFAULT(render_pkg)
{
    stbds_arrsetcap(this->mesh_pkgs_, 100);
    return this;
}

size_t render_pkg_get_max_mesh_pkgs(render_pkg* this)
{
    return stbds_arrcap(this->mesh_pkgs_);
}

mesh_pkg* render_pkg_add_mesh_pkg(render_pkg* this)
{
    stbds_arrput(this->mesh_pkgs_, (mesh_pkg){});
    return this->mesh_pkgs_ + stbds_arrlen(this->mesh_pkgs_) - 1;
}

void render_pkg_reset(render_pkg* this)
{
    stbds_arrsetlen(this->mesh_pkgs_, 0);
}
