#include <iostream>

#include "graphics/vk_base.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/texture.hpp"
#include "graphics/buffer.hpp"
#include "graphics/mesh.hpp"

#include "scene/scene.hpp"
#include "extensions/loader.hpp"

int main(int argc, char** argv)
{
    Graphics g(1920, 1080, true);
    Swapchain swapchain;
    swapchain.create();

    ExtensionLoader model_loader_dll("exe/model_loader.dll");
    std::unique_ptr<Extension> model_loader = model_loader_dll.load_extension();
    Model cube("res/models/sponza/sponza.obj", model_loader);
    CpuTimer timer;

    Fence frame_fence;
    Semaphore image_aquired_sem;
    Semaphore submit_sem;

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.queueFamilyIndex = g.queue_indices(GRAPHICS_QUEUE_IDX);
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::CommandPool main_pool = g.device().createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.commandPool = main_pool;
    alloc_info.commandBufferCount = 1;
    alloc_info.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmd = g.device().allocateCommandBuffers(alloc_info)[0];

    while (g.running())
    {
        frame_fence.wait();
        timer.end();

        uint32_t image_idx = swapchain.aquire_next_image(image_aquired_sem);
        g.device().resetFences(frame_fence);

        cmd.reset();
        begin_cmd(cmd);
        cmd.end();

        vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eBottomOfPipe};
        vk::SubmitInfo submit_info{};
        submit_info.setCommandBuffers(cmd);
        submit_info.setWaitDstStageMask(wait_stages);
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &submit_sem;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &image_aquired_sem;
        g.queues(GRAPHICS_QUEUE_IDX).submit(submit_info, frame_fence);
        swapchain.present({submit_sem});
        timer.start();
    }
    g.device().waitIdle();
    g.device().destroyCommandPool(main_pool);
    swapchain.destory();

    return EXIT_SUCCESS;
}