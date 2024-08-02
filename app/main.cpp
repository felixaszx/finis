#include <iostream>

#include "extensions/loader.hpp"
#include "graphics2/graphics.hpp"
#include "graphics2/swapchain.hpp"
#include "graphics2/res_uploader.hpp"
#include "fl_ext.hpp"

int main(int argc, char** argv)
{
    using namespace fi;

    fle::DoubleWindow fltk(800, 600, "Test window");
    fltk.end();
    fle::Flow flow(0, 0, 800, 600);
    fltk.add(flow);
    fltk.resizable(flow);
    fltk.show();

    Graphics g(1920, 1080, "finis");
    Swapchain sc;
    sc.create();

    Semaphore next_img;
    Semaphore submit;
    Fence frame_fence;

    ResDetails sponza("res/models/sponza/Sponza.gltf");

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = g.queue_indices(Graphics::GRAPHICS);
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.commandBufferCount = 1;
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.level = vk::CommandBufferLevel::ePrimary;
    auto cmds = g.device().allocateCommandBuffers(cmd_alloc);

    while (g.update())
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);

        while (fle::Global::check(), glfwGetWindowAttrib(g.window(), GLFW_ICONIFIED))
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
            g.update();
        }

        if (glfwGetKey(g.window(), GLFW_KEY_P))
        {
            fltk.show();
        }

        cmds[0].reset();
        begin_cmd(cmds[0]);
        cmds[0].end();

        std::vector<vk::PipelineStageFlags> waiting_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        std::vector<vk::Semaphore> submit_sems = {submit};
        std::vector<vk::Semaphore> wait_sems = {next_img};
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

    fltk.hide();
    fle::Global::check();
    return EXIT_SUCCESS;
}