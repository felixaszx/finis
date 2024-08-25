#include <cmath>
#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/prims.hpp"
#include "graphics/swapchain.hpp"

int main(int argc, char** argv)
{
    using namespace fi;
    using namespace glms::literal;
    using namespace std::chrono_literals;

    const uint32_t WIN_WIDTH = 1920;
    const uint32_t WIN_HEIGHT = 1080;

    Graphics g(WIN_WIDTH, WIN_HEIGHT, "finis");
    Swapchain sc;
    sc.create();

    Primitives prims(20_mb, 2000);

    Semaphore next_img;
    Semaphore submit;
    Fence frame_fence;

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = g.queue_indices(Graphics::GRAPHICS);
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

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
        begin_cmd(cmds[0]);
        cmds[0].end();

        std::array<vk::PipelineStageFlags, 1> waiting_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        std::array<vk::Semaphore, 1> submit_sems = {submit};
        std::array<vk::Semaphore, 1> wait_sems = {next_img};
        vk::SubmitInfo submit_info{};
        submit_info.setSignalSemaphores(submit_sems);
        submit_info.setWaitSemaphores(wait_sems);
        submit_info.setWaitDstStageMask(waiting_stages);
        submit_info.setCommandBuffers(cmds);
        g.queues(GraphicsObject::GRAPHICS).submit(submit_info, frame_fence);
        sc.present(submit_sems);
    }
    g.device().waitIdle();

    sc.destory();
    g.device().destroyCommandPool(cmd_pool);

    return EXIT_SUCCESS;
}