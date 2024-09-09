#include <cmath>
#include <iostream>

#include "extensions/dll.hpp"
#include "graphics/graphics.hpp"
#include "graphics/prims.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/shader.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/textures.hpp"
#include "graphics/prim_res.hpp"
#include "resources/gltf_file.hpp"
#include "resources/gltf_structure.hpp"
#include "mgrs/render_mgr.hpp"

int main(int argc, char** argv)
{
    using namespace fi;
    using namespace glms::literals;
    using namespace util::literals;
    using namespace std::chrono_literals;

    const uint32_t WIN_WIDTH = 1920;
    const uint32_t WIN_HEIGHT = 1080;

    gfx::context g(WIN_WIDTH, WIN_HEIGHT, "finis");
    gfx::swapchain sc;
    sc.create();

    ext::dll render_dll("exe/render_mgr.dll");
    auto render_mgr = render_dll.load_unique<mgr::render>();
    render_mgr->construct(sc);
    std::function<bool()> render_func = render_mgr->get_frame_func();

    ext::dll res0_dll("exe/res0.dll");
    auto res0 = res0_dll.load_unique<gfx::prim_res>();
    mgr::render_pkg res0_pkg;
    res0_pkg.prims_ = res0->get_primitives();
    res0_pkg.skins_ = res0->get_prim_skin();
    res0_pkg.structs_ = res0->get_prim_structure();
    res0_pkg.tex_arr_ = res0->get_tex_arr();

    ext::dll pl0_dll("exe/pl0.dll");
    auto pipeline0 = pl0_dll.load_unique<gfx::gfx_pipeline>();
    mgr::pipeline pl_pkg;
    pl_pkg.pipeline_ = pipeline0->pipeline_;
    pl_pkg.layout_ = pipeline0->layout_;
    pl_pkg.pkg_idxs_.push_back(0);

    render_mgr->pkgs_.push_back(res0_pkg);
    render_mgr->pipelines_.push_back(pl_pkg);

    while (render_func())
    {
    }

    g.device().waitIdle();

    sc.destory();

    return EXIT_SUCCESS;
}