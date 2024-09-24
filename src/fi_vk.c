#include "fi_vk.h"
#include <volk.c>

void resize_callback(GLFWwindow* win, int width, int height)
{
    vk_ctx* ctx = glfwGetWindowUserPointer(win);
    sem_wait(&ctx->recreate_done_);
    sem_post(&ctx->resize_done_);

    ctx->width_ = width;
    ctx->height_ = height;
}

IMPL_OBJ_NEW(vk_ctx, uint32_t width, uint32_t height, bool full_screen)
{
    this->width_ = width;
    this->height_ = height;
    sem_init(&this->resize_done_, 0, 0);
    sem_init(&this->recreate_done_, 0, 1);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (full_screen)
    {
        this->win_ = glfwCreateWindow(width, height, "", glfwGetPrimaryMonitor(), nullptr);
    }
    else
    {
        this->win_ = glfwCreateWindow(width, height, "", nullptr, nullptr);
    }
    glfwSetWindowUserPointer(this->win_, this);
    glfwSetFramebufferSizeCallback(this->win_, resize_callback);

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    volkInitialize();
    VkApplicationInfo app_info = {};
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.ppEnabledExtensionNames = glfw_exts;
    instance_create_info.enabledExtensionCount = glfw_ext_count;
    vkCreateInstance(&instance_create_info, nullptr, &this->instance_);
    volkLoadInstance(this->instance_);
    glfwCreateWindowSurface(this->instance_, this->win_, nullptr, &this->surface_);

    uint32_t phy_d_count = 0;
    QUICK_GET(VkPhysicalDevice, vkEnumeratePhysicalDevices, this->instance_, &phy_d_count, phy_d);
    this->physical_ = phy_d[0];
    for (size_t p = 0; p < phy_d_count; p++)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(phy_d[p], &properties);
        this->physical_ = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? phy_d[p] : this->physical_;
    }
    ffree(phy_d);

    this->queue_idx_ = 0;
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceVulkan11Features feature11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    feature11.multiview = true;
    feature11.variablePointersStorageBuffer = true;
    feature11.uniformAndStorageBuffer16BitAccess = true;
    feature11.variablePointers = true;
    feature11.shaderDrawParameters = true;
    VkPhysicalDeviceVulkan12Features feature12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    feature12.bufferDeviceAddress = true;
    feature12.scalarBlockLayout = true;
    feature12.shaderInt8 = true;
    feature12.storageBuffer8BitAccess = true;
    feature12.shaderFloat16 = true;
    feature12.uniformAndStorageBuffer8BitAccess = true;
    feature12.runtimeDescriptorArray = true;
    feature12.descriptorIndexing = true;
    feature12.descriptorBindingVariableDescriptorCount = true;
    feature12.descriptorBindingPartiallyBound = true;
    feature12.descriptorBindingSampledImageUpdateAfterBind = true;
    feature12.descriptorBindingStorageBufferUpdateAfterBind = true;
    feature12.descriptorBindingStorageImageUpdateAfterBind = true;
    feature12.descriptorBindingStorageTexelBufferUpdateAfterBind = true;
    feature12.descriptorBindingUniformBufferUpdateAfterBind = true;
    VkPhysicalDeviceVulkan13Features feature13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    feature13.dynamicRendering = true;
    feature13.synchronization2 = true;

    VkPhysicalDeviceFeatures2 feature2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    feature2.features.samplerAnisotropy = true;
    feature2.features.fillModeNonSolid = true;
    feature2.features.geometryShader = true;
    feature2.features.fillModeNonSolid = true;
    feature2.features.multiDrawIndirect = true;
    feature2.features.shaderInt64 = true;
    feature2.features.shaderInt16 = true;
    feature2.features.shaderSampledImageArrayDynamicIndexing = true;
    feature2.pNext = &feature11;
    feature11.pNext = &feature12;
    feature12.pNext = &feature13;

    const char* device_ext_names[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.pNext = &feature2;
    device_create_info.pQueueCreateInfos = &queue_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.ppEnabledExtensionNames = device_ext_names;
    device_create_info.enabledExtensionCount = 1;

    vkCreateDevice(this->physical_, &device_create_info, nullptr, &this->device_);
    vkGetDeviceQueue(this->device_, this->queue_idx_, 0, &this->queue_);
    volkLoadDevice(this->device_);

    VkPipelineCacheCreateInfo pl_cache = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    vkCreatePipelineCache(this->device_, &pl_cache, nullptr, &this->pipeline_cache_);

    VmaVulkanFunctions vma_funcs = {};
    vma_funcs.vkAllocateMemory = vkAllocateMemory;
    vma_funcs.vkBindBufferMemory = vkBindBufferMemory;
    vma_funcs.vkBindImageMemory = vkBindImageMemory;
    vma_funcs.vkCreateBuffer = vkCreateBuffer;
    vma_funcs.vkCreateImage = vkCreateImage;
    vma_funcs.vkDestroyBuffer = vkDestroyBuffer;
    vma_funcs.vkDestroyImage = vkDestroyImage;
    vma_funcs.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vma_funcs.vkFreeMemory = vkFreeMemory;
    vma_funcs.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vma_funcs.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vma_funcs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vma_funcs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vma_funcs.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vma_funcs.vkMapMemory = vkMapMemory;
    vma_funcs.vkUnmapMemory = vkUnmapMemory;
    vma_funcs.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vma_funcs.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vma_funcs.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vma_funcs.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vma_funcs.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
    vma_funcs.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vma_funcs.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vma_funcs.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo vma_cinfo = {};
    vma_cinfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vma_cinfo.vulkanApiVersion = app_info.apiVersion;
    vma_cinfo.pVulkanFunctions = &vma_funcs;
    vma_cinfo.instance = this->instance_;
    vma_cinfo.physicalDevice = this->physical_;
    vma_cinfo.device = this->device_;
    vmaCreateAllocator(&vma_cinfo, &this->allocator_);
    return this;
}

IMPL_OBJ_DELETE(vk_ctx)
{
    vkDeviceWaitIdle(this->device_);

    vmaDestroyAllocator(this->allocator_);
    vkDestroyPipelineCache(this->device_, this->pipeline_cache_, nullptr);
    vkDestroyDevice(this->device_, nullptr);
    vkDestroySurfaceKHR(this->instance_, this->surface_, nullptr);
    vkDestroyInstance(this->instance_, nullptr);
    glfwDestroyWindow(this->win_);
    glfwTerminate();

    sem_destroy(&this->resize_done_);
    sem_destroy(&this->recreate_done_);
}

bool vk_ctx_update(vk_ctx* ctx)
{
    glfwPollEvents();
    return !glfwWindowShouldClose(ctx->win_);
}

IMPL_OBJ_NEW(vk_swapchain, vk_ctx* ctx)
{
    this->ctx_ = ctx;

    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = 0;
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, &this->recreate_fence_);

    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_, ctx->surface_, &capabilities);

    VkSwapchainCreateInfoKHR swapchain_cinfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_cinfo.surface = ctx->surface_;
    swapchain_cinfo.minImageCount = 3;
    swapchain_cinfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    swapchain_cinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_cinfo.imageExtent = capabilities.currentExtent;
    swapchain_cinfo.imageArrayLayers = 1;
    swapchain_cinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | //
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |     //
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchain_cinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_cinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_cinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_cinfo.clipped = true;
    swapchain_cinfo.oldSwapchain = nullptr;
    vkCreateSwapchainKHR(ctx->device_, &swapchain_cinfo, nullptr, &this->swapchain_);

    this->image_count_ = swapchain_cinfo.minImageCount;
    this->images_ = alloc(VkImage, this->image_count_);
    vkGetSwapchainImagesKHR(ctx->device_, this->swapchain_, &this->image_count_, this->images_);

    VkCommandPool cmd_pool = {};
    VkCommandPoolCreateInfo pool_cinfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, nullptr, &cmd_pool);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = cmd_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device_, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    for (size_t i = 0; i < this->image_count_; i++)
    {
        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = this->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(ctx->queue_, 1, &submit, this->recreate_fence_);
    vkWaitForFences(ctx->device_, 1, &this->recreate_fence_, true, UINT64_MAX);
    vkResetFences(this->ctx_->device_, 1, &this->recreate_fence_);

    vkDestroyCommandPool(ctx->device_, cmd_pool, nullptr);
    this->extent_ = swapchain_cinfo.imageExtent;
    return this;
}

IMPL_OBJ_DELETE(vk_swapchain)
{
    ffree(this->images_);
    vkDestroySwapchainKHR(this->ctx_->device_, this->swapchain_, nullptr);
    vkDestroyFence(this->ctx_->device_, this->recreate_fence_, nullptr);
}

bool vk_swapchain_recreate(vk_swapchain* this, VkCommandPool cmd_pool)
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->ctx_->physical_, this->ctx_->surface_, &capabilities);
    if (!capabilities.currentExtent.width || !capabilities.currentExtent.height)
    {
        return false;
    }

    vkDestroySwapchainKHR(this->ctx_->device_, this->swapchain_, nullptr);
    VkSwapchainCreateInfoKHR swapchain_cinfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_cinfo.surface = this->ctx_->surface_;
    swapchain_cinfo.minImageCount = this->image_count_;
    swapchain_cinfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    swapchain_cinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_cinfo.imageExtent = capabilities.currentExtent;
    swapchain_cinfo.imageArrayLayers = 1;
    swapchain_cinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | //
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |     //
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchain_cinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_cinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_cinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_cinfo.clipped = true;
    swapchain_cinfo.oldSwapchain = nullptr;
    vkCreateSwapchainKHR(this->ctx_->device_, &swapchain_cinfo, nullptr, &this->swapchain_);
    vkGetSwapchainImagesKHR(this->ctx_->device_, this->swapchain_, &this->image_count_, this->images_);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = cmd_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(this->ctx_->device_, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    for (size_t i = 0; i < this->image_count_; i++)
    {
        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = this->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(this->ctx_->queue_, 1, &submit, this->recreate_fence_);
    vkWaitForFences(this->ctx_->device_, 1, &this->recreate_fence_, true, UINT64_MAX);
    vkResetFences(this->ctx_->device_, 1, &this->recreate_fence_);

    this->extent_ = swapchain_cinfo.imageExtent;
    return true;
}

VkResult vk_swapchain_process(vk_swapchain* this,
                              VkCommandPool cmd_pool,
                              VkSemaphore signal,
                              VkFence fence,
                              uint32_t* image_idx)
{
    VkResult result = vkAcquireNextImageKHR(this->ctx_->device_, this->swapchain_, UINT64_MAX, //
                                            signal, fence, image_idx);

    switch (result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        {
            while (true)
            {
                sem_wait(&this->ctx_->resize_done_);
                if (vk_swapchain_recreate(this, cmd_pool))
                {
                    result = vkAcquireNextImageKHR(this->ctx_->device_, this->swapchain_, UINT64_MAX, signal, fence,
                                                   image_idx);
                    sem_post(&this->ctx_->recreate_done_);
                    break;
                }
                sem_post(&this->ctx_->recreate_done_);
            }
            break;
        }
        case VK_SUBOPTIMAL_KHR:
        {
            static const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &signal;
            submit.pWaitDstStageMask = &stage;
            vkQueueSubmit(this->ctx_->queue_, 1, &submit, this->recreate_fence_);
            vkWaitForFences(this->ctx_->device_, 1, &this->recreate_fence_, true, UINT64_MAX);
            vkResetFences(this->ctx_->device_, 1, &this->recreate_fence_);

            sem_wait(&this->ctx_->resize_done_);
            vk_swapchain_recreate(this, cmd_pool);
            result = vkAcquireNextImageKHR(this->ctx_->device_, this->swapchain_, UINT64_MAX, signal, fence, image_idx);
            sem_post(&this->ctx_->recreate_done_);
            break;
        }
        default:
            break;
    }
    return result;
}

VkSemaphoreSubmitInfo vk_get_sem_info(VkSemaphore sem, VkPipelineStageFlags2 stage)
{
    VkSemaphoreSubmitInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    info.semaphore = sem;
    info.stageMask = stage;
    return info;
}