#include "vk.h"

vk_ctx* new_vk_ctx(uint32_t width, uint32_t height)
{
    vk_ctx* ctx = alloc(vk_ctx);
    init_vk_ctx(ctx, width, height);
    return ctx;
}

void init_vk_ctx(vk_ctx* ctx, uint32_t width, uint32_t height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    ctx->win_ = glfwCreateWindow(width, height, "", nullptr, nullptr);

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    VkApplicationInfo app_info = {};
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.ppEnabledExtensionNames = glfw_exts;
    instance_create_info.enabledExtensionCount = glfw_ext_count;
    vkCreateInstance(&instance_create_info, nullptr, &ctx->instance_);
    glfwCreateWindowSurface(ctx->instance_, ctx->win_, nullptr, &ctx->surface_);

    uint32_t phy_d_count = 0;
    QUICK_GET(VkPhysicalDevice, vkEnumeratePhysicalDevices, ctx->instance_, &phy_d_count, phy_d);
    ctx->physical_ = phy_d[0];
    for (size_t p = 0; p < phy_d_count; p++)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(phy_d[p], &properties);
        ctx->physical_ = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? phy_d[p] : ctx->physical_;
    }
    ffree(phy_d);

    ctx->queue_idx_ = 0;
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
    feature12.runtimeDescriptorArray = true;
    feature12.bufferDeviceAddress = true;
    feature12.scalarBlockLayout = true;
    feature12.shaderInt8 = true;
    feature12.shaderFloat16 = true;
    feature12.uniformAndStorageBuffer8BitAccess = true;
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

    vkCreateDevice(ctx->physical_, &device_create_info, nullptr, &ctx->device_);
    vkGetDeviceQueue(ctx->device_, ctx->queue_idx_, 0, &ctx->queue_);

    VkPipelineCacheCreateInfo pl_cache = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    vkCreatePipelineCache(ctx->device_, &pl_cache, nullptr, &ctx->pipeline_cache_);

    VmaVulkanFunctions vma_funs = {};
    vma_funs.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vma_funs.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    vma_funs.vkGetDeviceBufferMemoryRequirements = &vkGetDeviceBufferMemoryRequirements;
    vma_funs.vkGetDeviceImageMemoryRequirements = &vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo vma_cinfo = {};
    vma_cinfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vma_cinfo.vulkanApiVersion = app_info.apiVersion;
    vma_cinfo.pVulkanFunctions = &vma_funs;
    vma_cinfo.instance = ctx->instance_;
    vma_cinfo.physicalDevice = ctx->physical_;
    vma_cinfo.device = ctx->device_;
    vmaCreateAllocator(&vma_cinfo, &ctx->allocator_);
}

void release_vk_ctx(vk_ctx* ctx)
{
    vkDeviceWaitIdle(ctx->device_);

    vmaDestroyAllocator(ctx->allocator_);
    vkDestroyPipelineCache(ctx->device_, ctx->pipeline_cache_, nullptr);
    vkDestroyDevice(ctx->device_, nullptr);
    vkDestroySurfaceKHR(ctx->instance_, ctx->surface_, nullptr);
    vkDestroyInstance(ctx->instance_, nullptr);
    glfwDestroyWindow(ctx->win_);
    glfwTerminate();
}

bool vk_ctx_update(vk_ctx* ctx)
{
    glfwPollEvents();
    return !glfwWindowShouldClose(ctx->win_);
}

VkSemaphoreSubmitInfo get_vk_sem_info(VkSemaphore sem, VkPipelineStageFlags2 stage)
{
    VkSemaphoreSubmitInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    info.semaphore = sem;
    info.stageMask = stage;
    return info;
}

vk_swapchain* new_vk_swapchain(vk_ctx* ctx, VkExtent2D extent)
{
    vk_swapchain* swapchain = alloc(vk_swapchain);
    init_vk_swapchain(swapchain, ctx, extent);
    return swapchain;
}

void init_vk_swapchain(vk_swapchain* swapchain, vk_ctx* ctx, VkExtent2D extent)
{
    swapchain->ctx_ = ctx;
    VkSwapchainCreateInfoKHR swapchain_cinfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_cinfo.surface = ctx->surface_;
    swapchain_cinfo.minImageCount = 3;
    swapchain_cinfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    swapchain_cinfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_cinfo.imageExtent = extent;
    swapchain_cinfo.imageArrayLayers = 1;
    swapchain_cinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | //
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |     //
                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    swapchain_cinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_cinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_cinfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    swapchain_cinfo.clipped = true;
    swapchain_cinfo.oldSwapchain = nullptr;
    vkCreateSwapchainKHR(ctx->device_, &swapchain_cinfo, nullptr, &swapchain->swapchain_);

    swapchain->image_count_ = 3;
    swapchain->images_ = alloc(VkImage, swapchain->image_count_);
    vkGetSwapchainImagesKHR(ctx->device_, swapchain->swapchain_, &swapchain->image_count_, swapchain->images_);

    VkFence fence = {};
    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, &fence);
    vkResetFences(ctx->device_, 1, &fence);

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
    for (size_t i = 0; i < swapchain->image_count_; i++)
    {
        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = swapchain->images_[i];
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);
    }
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(ctx->queue_, 1, &submit, fence);
    vkWaitForFences(ctx->device_, 1, &fence, true, UINT64_MAX);

    vkDestroyFence(ctx->device_, fence, nullptr);
    vkDestroyCommandPool(ctx->device_, cmd_pool, nullptr);
}

void release_vk_swapchain(vk_swapchain* swapchain)
{
    ffree(swapchain->images_);
    vkDestroySwapchainKHR(swapchain->ctx_->device_, swapchain->swapchain_, nullptr);
}