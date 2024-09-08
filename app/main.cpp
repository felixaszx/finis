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

int main(int argc, char** argv)
{
    using namespace fi;
    using namespace glms::literals;
    using namespace util::literals;
    using namespace std::chrono_literals;

    util::err("test");
    const uint32_t WIN_WIDTH = 1920;
    const uint32_t WIN_HEIGHT = 1080;

    gfx::context g(WIN_WIDTH, WIN_HEIGHT, "finis");
    gfx::swapchain sc;
    sc.create();

    ext::dll res0_dll("exe/res0.dll");
    auto res0 = res0_dll.load_unique<gfx::prim_res>();

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = g.queue_indices(gfx::context::GRAPHICS);
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.commandBufferCount = 1;
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.level = vk::CommandBufferLevel::ePrimary;
    auto cmds = g.device().allocateCommandBuffers(cmd_alloc);

    gfx::cpu_clock clock;
    gfx::semaphore next_img;
    gfx::semaphore submit;
    gfx::fence frame_fence;
    while (true)
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);

        gfx::cpu_clock::time_pt curr_time = clock.get_elapsed();

        while (glfwGetWindowAttrib(g.window(), GLFW_ICONIFIED))
        {
            std::this_thread::sleep_for(1ms);
            g.update();
        }
        if (!g.update())
        {
            break;
        }

        cmds[0].reset();
        vk::CommandBufferBeginInfo begin_info{};
        cmds[0].begin(begin_info);
        cmds[0].end();

        vk::CommandBufferSubmitInfo cmd_submit{.commandBuffer = cmds[0]};
        vk::SemaphoreSubmitInfo signal_submit = submit.submit_info(vk::PipelineStageFlagBits2::eBottomOfPipe);
        vk::SemaphoreSubmitInfo waite_submit = next_img.submit_info(vk::PipelineStageFlagBits2::eBottomOfPipe);
        vk::SubmitInfo2 submit2{};
        submit2.setCommandBufferInfos(cmd_submit);
        submit2.setSignalSemaphoreInfos(signal_submit);
        submit2.setWaitSemaphoreInfos(waite_submit);
        g.queues(gfx::context::GRAPHICS).submit2(submit2, frame_fence);
        sc.present({submit});
    }
    g.device().waitIdle();

    sc.destory();
    g.device().destroyCommandPool(cmd_pool);

    return EXIT_SUCCESS;
}