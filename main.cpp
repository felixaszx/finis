#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/buffer.hpp"
#include "graphics/render_mgr.hpp"

int main(int argc, char** argv)
{
    using namespace fi;

    Graphics g(1920, 1080, true);
    Swapchain sc;
    sc.create();

    TextureMgr texture_mgr;
    PipelineMgr pipeline_mgr;
    RenderMgr render_mgr;
    auto r_arr = render_mgr.upload_res("res/models/sponza_gltf/sponza.glb", texture_mgr);
    render_mgr.lock_and_prepared();

    while (g.update())
    {
    }

    sc.destory();
    return EXIT_SUCCESS;
}