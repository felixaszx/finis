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
    auto res1 = res0_dll.load_unique<gfx::prim_res>();

    ext::dll pl0_dll("exe/pl0.dll");
    auto pipeline0 = pl0_dll.load_unique<gfx::gfx_pipeline>();
    pipeline0->pkgs_.push_back(res0->get_pipeline_pkg());
    pipeline0->pkgs_.push_back(res1->get_pipeline_pkg());
    pipeline0->construct();

    ext::dll render_dll("exe/render_mgr.dll");
    auto render_mgr = render_dll.load_unique<mgr::render>();
    mgr::render::func render_func = render_mgr->get_frame_func();
    render_mgr->pipelines_.push_back(pipeline0.get());
    render_mgr->construct();

    gfx::semaphore next_img;
    gfx::semaphore submit;
    vk::SemaphoreSubmitInfo wait_info{.semaphore = next_img, //
                                      .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe};
    vk::SemaphoreSubmitInfo signal_info{.semaphore = submit, //
                                        .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe};

    while (g.update())
    {
        if (glfwGetWindowAttrib(g.window(), GLFW_ICONIFIED))
        {
            std::this_thread::sleep_for(1ms);
            continue;
        }

        uint32_t img_idx = 0;
        render_func({wait_info},   //
                    {signal_info}, //
                    [&]() { sc.aquire_next_image(img_idx, next_img); });
        sc.present({submit});
    }

    g.device().waitIdle();
    sc.destory();

    return EXIT_SUCCESS;
}