#include <cmath>
#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/prims.hpp"
#include "graphics/shader.hpp"
#include "graphics/swapchain.hpp"
#include "bs_th_pool/BS_thread_pool.hpp"
int main(int argc, char** argv)
{
    using namespace fi;
    using namespace glms::literal;
    using namespace std::chrono_literals;
    using namespace fi::graphics;

    const uint32_t WIN_WIDTH = 1920;
    const uint32_t WIN_HEIGHT = 1080;

    Graphics g(WIN_WIDTH, WIN_HEIGHT, "finis");
    Swapchain sc;
    sc.create();

    Shader pipeline_shader("res/shaders/test.slang");

    Semaphore next_img;
    Semaphore submit;
    Fence frame_fence;

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = g.queue_indices(Graphics::GRAPHICS);
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

    Primitives prims(20_mb, 2000);
    prims.generate_staging_buffer(10_kb);

    vk::CommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.commandBufferCount = 1;
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.level = vk::CommandBufferLevel::ePrimary;
    auto cmds = g.device().allocateCommandBuffers(cmd_alloc);

    CpuClock clock;
    while (true)
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);

        CpuClock::TimePoint curr_time = clock.get_elapsed();

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
        g.queues(GraphicsObject::GRAPHICS).submit2(submit2, frame_fence);
        sc.present({submit});
    }
    g.device().waitIdle();

    sc.destory();
    g.device().destroyCommandPool(cmd_pool);

    return EXIT_SUCCESS;
}