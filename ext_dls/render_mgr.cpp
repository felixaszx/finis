#include "mgrs/render_mgr.hpp"

using namespace fi;
using namespace glms::literals;
using namespace util::literals;
using namespace std::chrono_literals;

struct render : public mgr::render
{
    vk::CommandBuffer cmd_ = nullptr;
    vk::CommandPool cmd_pool_ = nullptr;

    gfx::cpu_clock clock_{};
    vk::Semaphore next_img_ = nullptr;
    vk::Semaphore submit_ = nullptr;
    vk::Fence frame_fence_ = nullptr;

    std::function<bool()> frame_func_ = [&]()
    {
        glfwPollEvents();
        auto r = device().waitForFences(frame_fence_, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc->aquire_next_image(next_img_);
        device().resetFences(frame_fence_);
        gfx::cpu_clock::time_pt curr_time = clock_.get_elapsed();

        while (glfwGetWindowAttrib(window(), GLFW_ICONIFIED))
        {
            std::this_thread::sleep_for(1ms);
            glfwPollEvents();
        }
        if (glfwWindowShouldClose(window()))
        {
            return false;
        }

        cmd_.reset();
        vk::CommandBufferBeginInfo begin_info{};
        cmd_.begin(begin_info);
        cmd_.end();

        vk::SemaphoreSubmitInfo signal_sem{};
        signal_sem.setSemaphore(submit_);
        signal_sem.stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        vk::SemaphoreSubmitInfo swait_sem{};
        swait_sem.setSemaphore(next_img_);
        swait_sem.stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;

        vk::CommandBufferSubmitInfo cmd_submit{.commandBuffer = cmd_};
        vk::SubmitInfo2 submit2{};
        submit2.setCommandBufferInfos(cmd_submit);
        submit2.setSignalSemaphoreInfos(signal_sem);
        submit2.setWaitSemaphoreInfos(swait_sem);
        queues(gfx::context::GRAPHICS).submit2(submit2, frame_fence_);
        sc->present({submit_});
        return true;
    };

    ~render() override
    {
        device().destroyCommandPool(cmd_pool_);
        device().destroySemaphore(next_img_);
        device().destroySemaphore(submit_);
        device().destroyFence(frame_fence_);
    }

    void construct_derived() override
    {
        vk::CommandPoolCreateInfo pool_info{};
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = queue_indices(gfx::context::GRAPHICS);
        cmd_pool_ = device().createCommandPool(pool_info);

        vk::CommandBufferAllocateInfo cmd_alloc{};
        cmd_alloc.commandBufferCount = 1;
        cmd_alloc.commandPool = cmd_pool_;
        cmd_alloc.level = vk::CommandBufferLevel::ePrimary;
        cmd_ = device().allocateCommandBuffers(cmd_alloc)[0];

        vk::SemaphoreCreateInfo sem_info{};
        next_img_ = device().createSemaphore(sem_info);
        submit_ = device().createSemaphore(sem_info);

        vk::FenceCreateInfo fence_info{.flags = vk::FenceCreateFlagBits::eSignaled};
        frame_fence_ = device().createFence(fence_info);
    }

    std::function<bool()> get_frame_func() override { return frame_func_; }
};

EXPORT_EXTENSION(render);