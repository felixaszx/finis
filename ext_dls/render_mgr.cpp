#include "mgrs/render_mgr.hpp"

using namespace fi;
using namespace glms::literals;
using namespace util::literals;
using namespace std::chrono_literals;

struct render : public mgr::render
{
    vk::CommandBuffer cmd_ = nullptr;
    vk::CommandPool cmd_pool_ = nullptr;
    vk::Fence frame_fence_ = nullptr;

    func frame_func_ = [&](const std::vector<vk::SemaphoreSubmitInfo>& waits,
                           const std::vector<vk::SemaphoreSubmitInfo>& signals,
                           const std::function<void()>& deffered)
    {
        auto r = device().waitForFences(frame_fence_, true, std::numeric_limits<uint64_t>::max());
        if (deffered)
        {
            deffered();
        }
        device().resetFences(frame_fence_);

        cmd_.reset();
        vk::CommandBufferBeginInfo begin_info{};
        cmd_.begin(begin_info);
        cmd_.end();

        vk::CommandBufferSubmitInfo cmd_submit{.commandBuffer = cmd_};
        vk::SubmitInfo2 submit2{};
        submit2.setCommandBufferInfos(cmd_submit);
        submit2.setSignalSemaphoreInfos(signals);
        submit2.setWaitSemaphoreInfos(waits);
        queues(gfx::context::GRAPHICS).submit2(submit2, frame_fence_);
    };

    ~render() override
    {
        device().destroyCommandPool(cmd_pool_);
        device().destroyFence(frame_fence_);
    }

    void construct() override
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

        vk::FenceCreateInfo fence_info{.flags = vk::FenceCreateFlagBits::eSignaled};
        frame_fence_ = device().createFence(fence_info);
    }

    func get_frame_func() override { return frame_func_; }
};

EXPORT_EXTENSION(render);