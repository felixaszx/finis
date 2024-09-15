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

    void set_draw_func()
    {
        frame_func_ = [this](const std::vector<vk::SemaphoreSubmitInfo>& waits,
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
    }

    ~render() override
    {
        device().destroyCommandPool(cmd_pool_);
        device().destroyFence(frame_fence_);
        device().destroyDescriptorPool(desc_pool_);
    }

    void init()
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
        set_draw_func();
    }

    void construct() override
    {
        init();

        // creating nessary resources
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
            image_views_.push_back(device().createImageView(view_info));
            pipelines_[0]->atchm_infos_[i].imageView = image_views_.back();
        }

        atchm_info.format = vk::Format::eD24UnormS8Uint;
        atchm_info.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        auto image_alloc = allocator().createImage(atchm_info, alloc_info);
        images_.push_back(image_alloc.first);
        image_allocs_.push_back(image_alloc.second);

        vk::ImageViewCreateInfo view_info{.image = images_.back(),
                                          .viewType = vk::ImageViewType::e2D,
                                          .format = atchm_info.format,
                                          .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eDepth |  //
                                                                             vk::ImageAspectFlagBits::eStencil, //
                                                               .levelCount = 1,
                                                               .layerCount = 1}};
        image_views_.push_back(device().createImageView(view_info));
        pipelines_[0]->atchm_infos_.back().imageView = image_views_.back();
        pipelines_[0]->render_info_.layerCount = 1;

        vk::DeviceSize buffer_size = 0;
        for (auto& buffer_info : pipelines_[0]->shader_ref_->buffer_infos_)
        {
            buffer_info.offset = buffer_size;
            buffer_size += buffer_info.range;
        }

        vk::BufferCreateInfo buffer_info{.size = buffer_size, //
                                         .usage = vk::BufferUsageFlagBits::eUniformBuffer};
        vma::AllocationInfo buffer_alloc_info{};
        alloc_info.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | //
                           vma::AllocationCreateFlagBits::eMapped;
        auto buffer_alloc = allocator().createBuffer(buffer_info, alloc_info, buffer_alloc_info);
        buffers_.push_back(buffer_alloc.first);
        buffer_allocs_.push_back(buffer_alloc.second);
        buffer_mappings_.push_back((std::byte*)(buffer_alloc_info.pMappedData));
    }
};

REGISTER_EXT(render);