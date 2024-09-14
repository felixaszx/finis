#include "mgrs/render_mgr.hpp"

using namespace fi;
using namespace glms::literals;
using namespace util::literals;
using namespace std::chrono_literals;

struct render : public mgr::render
{
    vk::CommandBuffer cmd_{};
    vk::CommandPool cmd_pool_{};
    vk::Fence frame_fence_{};

    vk::DescriptorPool desc_pool_{};
    std::vector<std::vector<vk::DescriptorSet>> desc_sets_{}; // per pipeline

    ~render() override
    {
        for (size_t i = 0; i < images_.size(); i++)
        {
            allocator().destroyImage(images_[i], image_allocs_[i]);
        }

        for (vk::ImageView view : imag_views_)
        {
            device().destroyImageView(view);
        }

        for (size_t i = 0; i < buffers_.size(); i++)
        {
            allocator().destroyBuffer(buffers_[i], buffer_allocs_[i]);
        }

        device().destroyCommandPool(cmd_pool_);
        device().destroyFence(frame_fence_);
        device().destroyDescriptorPool(desc_pool_);
    }

    void construct() override
    {
        name_ = "render_mgr";

        uint32_t total_set = 0;
        std::unordered_map<vk::DescriptorType, uint32_t> desc_sizes{};
        for (const auto& pl : pipelines_)
        {
            desc_sizes.insert(pl->desc_sizes_.begin(), pl->desc_sizes_.end());
            total_set += pl->shader_ref_->desc_sets_.size();
        }

        std::vector<vk::DescriptorPoolSize> pool_sizes{};
        pool_sizes.reserve(desc_sizes.size());
        for (const auto& size : desc_sizes)
        {
            pool_sizes.emplace_back(size.first, size.second);
        }

        vk::DescriptorPoolCreateInfo desc_pool_info{};
        desc_pool_info.setPoolSizes(pool_sizes);
        desc_pool_info.maxSets = total_set;
        desc_pool_ = device().createDescriptorPool(desc_pool_info);

        desc_sets_.reserve(pipelines_.size());
        for (const auto& pl : pipelines_)
        {
            desc_sets_.push_back(pl->setup_desc_set(desc_pool_));
        }

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

        vk::ImageCreateInfo atchm_info{.imageType = vk::ImageType::e2D,
                                       .format = vk::Format::eR32G32B32A32Sfloat,
                                       .extent = vk::Extent3D(1920, 1080, 1),
                                       .mipLevels = 1,
                                       .arrayLayers = 1,
                                       .usage = vk::ImageUsageFlagBits::eColorAttachment};
        vma::AllocationCreateInfo alloc_info{.usage = vma::MemoryUsage::eAutoPreferDevice};

        for (int i = 0; i < 4; i++)
        {
            auto allocated = allocator().createImage(atchm_info, alloc_info);
            images_.push_back(allocated.first);
            image_allocs_.push_back(allocated.second);

            vk::ImageViewCreateInfo view_info{.image = images_.back(),
                                              .viewType = vk::ImageViewType::e2D,
                                              .format = atchm_info.format,
                                              .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, //
                                                                   .levelCount = 1,
                                                                   .layerCount = 1}};
            imag_views_.push_back(device().createImageView(view_info));
        }

        set_draw_func();
    }
    func draw_func_derived_ = [this](const std::vector<vk::SemaphoreSubmitInfo>& waits,
                                     const std::vector<vk::SemaphoreSubmitInfo>& signals, //
                                     const std::function<void()>& deffered)
    {
        auto r = device().waitForFences(frame_fence_, true, std::numeric_limits<uint64_t>::max());
        deffered();
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
    void set_draw_func() { frame_func_ = std::ref(draw_func_derived_); }
};

EXPORT_EXTENSION(render);