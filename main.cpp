#include <iostream>
#include "graphics/vk_base.hpp"
#include "graphics/resources.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/pass.hpp"
#include "scene/tree.hpp"

#include "extensions/loader.hpp"

int main(int argc, char** argv)
{
    SceneNode root;
    SceneNode l1;
    SceneNode l2;
    SceneNode l3;
    SceneNode l4;
    l1.set_parent(&root);
    l2.set_parent(&root);
    l3.set_parent(&l1);
    l4.set_parent(&l1);

    l1.translate({1, 0, 0});
    l2.translate({2, 0, 0});
    l3.translate({3, 0, 0});
    l4.translate({4, 0, 0});
    root.traverse_breath_first([](SceneNode& node) { std::cout << node.get_world_position()[0] << '\n'; });
    /*
    Graphics g(1920, 1080, true);
    ExtensionLoader test_ext("exe/test.dll");
    ExtensionLoader pass_ext("exe/g_buffer.dll");

    Swapchain sc;
    sc.create();

    Pass p(pass_ext.load_pass_funcs());

    PassGroup pg;
    pg.register_pass(test_ext.load_pass_funcs());
    pg.register_pass(test_ext.load_pass_funcs());
    pg.register_pass(test_ext.load_pass_funcs());

    Pass* p1 = pg.get_pass(0);
    Pass* p2 = pg.get_pass(1);
    Pass* p3 = pg.get_pass(2);

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.queueFamilyIndex = g.queue_indices(GRAPHICS_QUEUE_IDX);
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo cmd_info{};
    cmd_info.commandPool = cmd_pool;
    cmd_info.commandBufferCount = 1;
    cmd_info.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmd = g.device().allocateCommandBuffers(cmd_info)[0];

    Fence frame_fence;
    Semaphore sem_aquired;
    Semaphore submit;

    CpuTimer timer;

    while (!glfwWindowShouldClose(g.window()))
    {
        timer.start();
        glfwPollEvents();
        p.setup();

        TRY_FUNC
        auto r = g.device().waitForFences(frame_fence, true, UINT64_MAX);
        CATCH_FUNC

        g.device().resetFences(frame_fence);
        sc.aquire_next_image(sem_aquired, nullptr);

        cmd.reset();
        vk::CommandBufferBeginInfo begin_info{};
        cmd.begin(begin_info);
        cmd.end();

        std::array<vk::PipelineStageFlags, 1> wait_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submit_info{};
        submit_info.setWaitDstStageMask(wait_stages);
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &sem_aquired;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &submit;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd;
        g.queues(GRAPHICS_QUEUE_IDX).submit(submit_info, frame_fence);
        sc.present({submit});

        p.finish();
        timer.finish();
    }
    g.device().waitIdle();

    g.device().destroyCommandPool(cmd_pool);
    sc.destory();
    */

    return EXIT_SUCCESS;
}