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

    ext::dll res0_dll("exe/res0.dll");
    auto res0 = res0_dll.load_unique<gfx::prim_res>();

    ext::dll render_dll("exe/render_mgr.dll");
    auto render_mgr = render_dll.load_unique<mgr::render>();
    render_mgr->construct(sc);
    std::function<bool()> render_func = render_mgr->get_frame_func();

    while (render_func())
    {
    }

    g.device().waitIdle();

    sc.destory();

    return EXIT_SUCCESS;
}