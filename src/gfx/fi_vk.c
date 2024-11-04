#include "fi_vk.h"

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
    cthis->width_ = width;
    cthis->height_ = height;
    sem_init(&cthis->resize_done_, 0, 0);
    sem_init(&cthis->recreate_done_, 0, 1);

#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (full_screen)
    {
        cthis->win_ = glfwCreateWindow(width, height, "", glfwGetPrimaryMonitor(), fi_nullptr);
    }
    else
    {
        cthis->win_ = glfwCreateWindow(width, height, "", fi_nullptr, fi_nullptr);
    }
    glfwSetWindowUserPointer(cthis->win_, cthis);
    glfwSetFramebufferSizeCallback(cthis->win_, resize_callback);

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    VkApplicationInfo app_info = {};
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_create_info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.ppEnabledExtensionNames = glfw_exts;
    instance_create_info.enabledExtensionCount = glfw_ext_count;
    vkCreateInstance(&instance_create_info, fi_nullptr, &cthis->instance_);
    glfwCreateWindowSurface(cthis->instance_, cthis->win_, fi_nullptr, &cthis->surface_);

    uint32_t phy_d_count = 0;
    VkPhysicalDevice* phy_d = fi_nullptr;
    vkEnumeratePhysicalDevices(cthis->instance_, &phy_d_count, fi_nullptr);
    phy_d = malloc(phy_d_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(cthis->instance_, &phy_d_count, phy_d);
    cthis->physical_ = phy_d[0];

    for (size_t p = 0; p < phy_d_count; p++)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(phy_d[p], &properties);
        cthis->physical_ = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? phy_d[p] : cthis->physical_;
    }
    free(phy_d);
    phy_d = fi_nullptr;

    cthis->queue_idx_ = 0;
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceVulkan11Features feature11 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    feature11.multiview = true;
    feature11.variablePointersStorageBuffer = true;
    feature11.uniformAndStorageBuffer16BitAccess = true;
    feature11.variablePointers = true;
    feature11.shaderDrawParameters = true;
    VkPhysicalDeviceVulkan12Features feature12 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    feature12.bufferDeviceAddress = true;
    feature12.scalarBlockLayout = true;
    feature12.shaderInt8 = true;
    feature12.storageBuffer8BitAccess = true;
    feature12.shaderFloat16 = true;
    feature12.uniformAndStorageBuffer8BitAccess = true;
    feature12.descriptorIndexing = true;
    feature12.runtimeDescriptorArray = true;
    feature12.descriptorBindingVariableDescriptorCount = true;
    feature12.descriptorBindingPartiallyBound = true;
    feature12.descriptorBindingSampledImageUpdateAfterBind = true;
    feature12.descriptorBindingStorageBufferUpdateAfterBind = true;
    feature12.descriptorBindingStorageImageUpdateAfterBind = true;
    feature12.descriptorBindingStorageTexelBufferUpdateAfterBind = true;
    feature12.descriptorBindingUniformBufferUpdateAfterBind = true;
    VkPhysicalDeviceVulkan13Features feature13 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    feature13.dynamicRendering = true;
    feature13.synchronization2 = true;

    VkPhysicalDeviceFeatures2 feature2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
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
    VkDeviceCreateInfo device_create_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create_info.pNext = &feature2;
    device_create_info.pQueueCreateInfos = &queue_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.ppEnabledExtensionNames = device_ext_names;
    device_create_info.enabledExtensionCount = 1;

    vkCreateDevice(cthis->physical_, &device_create_info, fi_nullptr, &cthis->device_);
    vkGetDeviceQueue(cthis->device_, cthis->queue_idx_, 0, &cthis->queue_);

    VkPipelineCacheCreateInfo pl_cache = {.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    vkCreatePipelineCache(cthis->device_, &pl_cache, fi_nullptr, &cthis->pipeline_cache_);

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
    vma_funcs.vkBindImageMemory2KHR = vkBindImageMemory2;
    vma_funcs.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vma_funcs.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vma_funcs.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo vma_cinfo = {};
    vma_cinfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vma_cinfo.vulkanApiVersion = app_info.apiVersion;
    vma_cinfo.pVulkanFunctions = &vma_funcs;
    vma_cinfo.instance = cthis->instance_;
    vma_cinfo.physicalDevice = cthis->physical_;
    vma_cinfo.device = cthis->device_;
    vmaCreateAllocator(&vma_cinfo, &cthis->allocator_);
    return cthis;
}

IMPL_OBJ_DELETE(vk_ctx)
{
    vkDeviceWaitIdle(cthis->device_);

    vmaDestroyAllocator(cthis->allocator_);
    vkDestroyPipelineCache(cthis->device_, cthis->pipeline_cache_, fi_nullptr);
    vkDestroyDevice(cthis->device_, fi_nullptr);
    vkDestroySurfaceKHR(cthis->instance_, cthis->surface_, fi_nullptr);
    vkDestroyInstance(cthis->instance_, fi_nullptr);
    glfwDestroyWindow(cthis->win_);
    glfwTerminate();

    sem_destroy(&cthis->resize_done_);
    sem_destroy(&cthis->recreate_done_);
}

bool vk_ctx_update(vk_ctx* ctx)
{
    glfwPollEvents();
    return !glfwWindowShouldClose(ctx->win_);
}

IMPL_OBJ_NEW(vk_swapchain, vk_ctx* ctx)
{
    cthis->ctx_ = ctx;
    cthis->vsync_ = false;

    VkFenceCreateInfo fence_cinfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = 0;
    vkCreateFence(ctx->device_, &fence_cinfo, fi_nullptr, &cthis->recreate_fence_);

    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_, ctx->surface_, &capabilities);

    uint32_t format_count = 0;
    VkSurfaceFormatKHR* formats = fi_nullptr;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_, ctx->surface_, &format_count, fi_nullptr);
    formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_, ctx->surface_, &format_count, formats);

    VkSwapchainCreateInfoKHR swapchain_cinfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_cinfo.surface = ctx->surface_;
    swapchain_cinfo.minImageCount = 3;
    swapchain_cinfo.imageExtent = capabilities.currentExtent;
    swapchain_cinfo.imageArrayLayers = 1;
    swapchain_cinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | //
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |     //
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchain_cinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_cinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_cinfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    swapchain_cinfo.clipped = true;
    swapchain_cinfo.oldSwapchain = fi_nullptr;

    for (uint32_t i = 0; i < format_count; i++)
    {
        if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
        {
            swapchain_cinfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
            swapchain_cinfo.imageColorSpace = formats[i].colorSpace;
            break;
        }

        if (i == format_count - 1)
        {
            swapchain_cinfo.imageFormat = formats[0].format;
            swapchain_cinfo.imageColorSpace = formats[0].colorSpace;
        }
    }
    cthis->surface_format_.format = swapchain_cinfo.imageFormat;
    cthis->surface_format_.colorSpace = swapchain_cinfo.imageColorSpace;
    vkCreateSwapchainKHR(ctx->device_, &swapchain_cinfo, fi_nullptr, &cthis->swapchain_);
    free(formats);
    formats = fi_nullptr;

    cthis->image_count_ = swapchain_cinfo.minImageCount;
    cthis->images_ = fi_alloc(VkImage, cthis->image_count_);
    vkGetSwapchainImagesKHR(ctx->device_, cthis->swapchain_, &cthis->image_count_, cthis->images_);

    VkCommandPool cmd_pool = {};
    VkCommandPoolCreateInfo pool_cinfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, fi_nullptr, &cmd_pool);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = cmd_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device_, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    for (size_t i = 0; i < cthis->image_count_; i++)
    {
        VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = cthis->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //
                             0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);
    }
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(ctx->queue_, 1, &submit, cthis->recreate_fence_);
    vkWaitForFences(ctx->device_, 1, &cthis->recreate_fence_, true, UINT64_MAX);
    vkResetFences(cthis->ctx_->device_, 1, &cthis->recreate_fence_);

    vkDestroyCommandPool(ctx->device_, cmd_pool, fi_nullptr);
    cthis->extent_ = swapchain_cinfo.imageExtent;
    return cthis;
}

IMPL_OBJ_DELETE(vk_swapchain)
{
    fi_free(cthis->images_);
    vkDestroySwapchainKHR(cthis->ctx_->device_, cthis->swapchain_, fi_nullptr);
    vkDestroyFence(cthis->ctx_->device_, cthis->recreate_fence_, fi_nullptr);
}

bool vk_swapchain_recreate(vk_swapchain* cthis, VkCommandPool cmd_pool)
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(cthis->ctx_->physical_, cthis->ctx_->surface_, &capabilities);
    if (!capabilities.currentExtent.width || !capabilities.currentExtent.height)
    {
        return false;
    }

    vkDestroySwapchainKHR(cthis->ctx_->device_, cthis->swapchain_, fi_nullptr);
    VkSwapchainCreateInfoKHR swapchain_cinfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_cinfo.surface = cthis->ctx_->surface_;
    swapchain_cinfo.minImageCount = cthis->image_count_;
    swapchain_cinfo.imageFormat = cthis->surface_format_.format;
    swapchain_cinfo.imageColorSpace = cthis->surface_format_.colorSpace;
    swapchain_cinfo.imageExtent = capabilities.currentExtent;
    swapchain_cinfo.imageArrayLayers = 1;
    swapchain_cinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | //
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |     //
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchain_cinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_cinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_cinfo.presentMode = cthis->vsync_ ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    swapchain_cinfo.clipped = true;
    swapchain_cinfo.oldSwapchain = fi_nullptr;
    vkCreateSwapchainKHR(cthis->ctx_->device_, &swapchain_cinfo, fi_nullptr, &cthis->swapchain_);
    vkGetSwapchainImagesKHR(cthis->ctx_->device_, cthis->swapchain_, &cthis->image_count_, cthis->images_);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.commandPool = cmd_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(cthis->ctx_->device_, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin);
    for (size_t i = 0; i < cthis->image_count_; i++)
    {
        VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = cthis->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //
                             0, 0, fi_nullptr, 0, fi_nullptr, 1, &barrier);
    }
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(cthis->ctx_->queue_, 1, &submit, cthis->recreate_fence_);
    vkWaitForFences(cthis->ctx_->device_, 1, &cthis->recreate_fence_, true, UINT64_MAX);
    vkResetFences(cthis->ctx_->device_, 1, &cthis->recreate_fence_);

    cthis->extent_ = swapchain_cinfo.imageExtent;
    return true;
}

VkResult vk_swapchain_process(vk_swapchain* cthis,
                              VkCommandPool cmd_pool,
                              VkSemaphore signal,
                              VkFence fence,
                              uint32_t* image_idx)
{
    VkResult result = vkAcquireNextImageKHR(cthis->ctx_->device_, cthis->swapchain_, UINT64_MAX, //
                                            signal, fence, image_idx);

    switch (result)
    {
        case VK_ERROR_OUT_OF_DATE_KHR:
        {
            while (true)
            {
                sem_wait(&cthis->ctx_->resize_done_);
                if (vk_swapchain_recreate(cthis, cmd_pool))
                {
                    result = vkAcquireNextImageKHR(cthis->ctx_->device_, cthis->swapchain_, UINT64_MAX, signal, fence,
                                                   image_idx);
                    sem_post(&cthis->ctx_->recreate_done_);
                    break;
                }
                sem_post(&cthis->ctx_->recreate_done_);
            }
            break;
        }
        case VK_SUBOPTIMAL_KHR:
        {
            static const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &signal;
            submit.pWaitDstStageMask = &stage;
            vkQueueSubmit(cthis->ctx_->queue_, 1, &submit, cthis->recreate_fence_);
            vkWaitForFences(cthis->ctx_->device_, 1, &cthis->recreate_fence_, true, UINT64_MAX);
            vkResetFences(cthis->ctx_->device_, 1, &cthis->recreate_fence_);

            sem_wait(&cthis->ctx_->resize_done_);
            vk_swapchain_recreate(cthis, cmd_pool);
            result = vkAcquireNextImageKHR(cthis->ctx_->device_, cthis->swapchain_, UINT64_MAX, signal, fence, image_idx);
            sem_post(&cthis->ctx_->recreate_done_);
            break;
        }
        default:
            break;
    }
    return result;
}

VkSemaphoreSubmitInfo vk_get_sem_info(VkSemaphore sem, VkPipelineStageFlags2 stage)
{
    VkSemaphoreSubmitInfo info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    info.semaphore = sem;
    info.stageMask = stage;
    return info;
}