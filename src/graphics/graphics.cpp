/**
 * @file graphics.cpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#define VMA_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "graphics/graphics.hpp"
#include <glm/glm.hpp>

fi::graphics::Graphics::Graphics(int width, int height, const std::string& title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    std::array<const char*, 1> VALIDATION_LAYERS{"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> instance_exts{glfw_exts, glfw_exts + glfw_ext_count};

    vk::ApplicationInfo app_info{};
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    vk::InstanceCreateInfo instance_create_info{};
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.setPEnabledExtensionNames(instance_exts);
    instance_ = vk::createInstance(instance_create_info);

    VkSurfaceKHR surface = nullptr;
    glfwCreateWindowSurface(instance_, window_, nullptr, &surface);
    surface_ = surface;

    // now create device
    std::vector<vk::PhysicalDevice> physical_devices = instance_.enumeratePhysicalDevices();
    physical_ = physical_devices[0];
    for (auto& physical : physical_devices)
    {
        physical_ = (physical.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) //
                        ? physical
                        : physical_;
    }

    std::vector<vk::QueueFamilyProperties> queue_properties = physical_.getQueueFamilyProperties();
    uint32_t queue_count = 0;
    for (uint32_t index = 0; index < queue_properties.size(); index++)
    {
        if (queue_properties[index].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            queue_count++;
            queue_indices_[GRAPHICS] = index;
            break;
        }
    }
    queue_indices_[COMPUTE] = queue_indices_[GRAPHICS];
    queue_indices_[TRANSFER] = queue_indices_[GRAPHICS];

    for (uint32_t index = 0; index < queue_properties.size(); index++)
    {
        if (queue_properties[index].queueFlags & vk::QueueFlagBits::eCompute &&  //
            queue_properties[index].queueFlags & vk::QueueFlagBits::eTransfer && //
            index != queue_indices_[GRAPHICS])
        {
            queue_count++;
            queue_indices_[COMPUTE] = index;
            break;
        }
    }

    for (uint32_t index = 0; index < queue_properties.size(); index++)
    {
        if (queue_properties[index].queueFlags & vk::QueueFlagBits::eTransfer && //
            index != queue_indices_[GRAPHICS] &&                                 //
            index != queue_indices_[COMPUTE])
        {
            queue_count++;
            queue_indices_[TRANSFER] = index;
            break;
        }
    }

    float queue_priority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos{};
    for (int i = 0; i < queue_count; i++)
    {
        if (i > 0 && queue_indices_[i] == queue_indices_[i - 1])
        {
            break;
        }

        vk::DeviceQueueCreateInfo queue_create_info{};
        queue_create_info.queueFamilyIndex = queue_indices_[i];
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    vk::PhysicalDeviceFeatures feature{};
    feature.samplerAnisotropy = true;
    feature.fillModeNonSolid = true;
    feature.geometryShader = true;
    feature.fillModeNonSolid = true;
    feature.multiDrawIndirect = true;
    feature.shaderSampledImageArrayDynamicIndexing = true;
    vk::PhysicalDeviceVulkan11Features feature11{};
    feature11.multiview = true;
    feature11.shaderDrawParameters = true;
    vk::PhysicalDeviceVulkan12Features feature12{};
    feature12.runtimeDescriptorArray = true;
    feature12.bufferDeviceAddress = true;
    feature12.scalarBlockLayout = true;
    vk::PhysicalDeviceVulkan13Features feature13{};
    feature13.dynamicRendering = true;
    feature13.synchronization2 = true;

    vk::PhysicalDeviceFeatures2 feature_alt{};
    feature_alt.setFeatures(feature);
    feature_alt.pNext = &feature11;
    feature11.pNext = &feature12;
    feature12.pNext = &feature13;

    std::vector<const char*> device_ext_names = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char*> device_layer_names;

    vk::DeviceCreateInfo device_create_info{};
    device_create_info.pNext = &feature_alt;
    device_create_info.setQueueCreateInfos(queue_create_infos);
    device_create_info.setPEnabledExtensionNames(device_ext_names);
    device_create_info.setPEnabledLayerNames(device_layer_names);

    device_ = physical_.createDevice(device_create_info);
    for (int i = 0; i < 3; i++)
    {
        queues_[i] = device_.getQueue(queue_indices_[i], 0);
    }

    vma::VulkanFunctions vma_function{};
    vma_function.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vma_function.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    vma_function.vkGetDeviceBufferMemoryRequirements = &vkGetDeviceBufferMemoryRequirements;
    vma_function.vkGetDeviceImageMemoryRequirements = &vkGetDeviceImageMemoryRequirements;

    vma::AllocatorCreateInfo vma_create_info{};
    vma_create_info.flags = vma::AllocatorCreateFlagBits::eBufferDeviceAddress;
    vma_create_info.vulkanApiVersion = app_info.apiVersion;
    vma_create_info.pVulkanFunctions = &vma_function;
    vma_create_info.instance = instance_;
    vma_create_info.physicalDevice = physical_;
    vma_create_info.device = device_;
    allocator_ = vma::createAllocator(vma_create_info);

    vk::PipelineCacheCreateInfo pc_info{};
    pipeline_cache_ = device_.createPipelineCache(pc_info);
}

fi::graphics::Graphics::~Graphics()
{
    device_.waitIdle();

    device().destroyPipelineCache(pipeline_cache_);
    instance_.destroySurfaceKHR(surface_);
    allocator_.destroy();
    device_.destroy();

    instance_.destroy();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool fi::graphics::Graphics::update()
{
    bool running = !glfwWindowShouldClose(window_);
    glfwPollEvents();
    return running;
}

vk::Instance fi::graphics::GraphicsObject::instance()
{
    return instance_;
};

vk::SurfaceKHR fi::graphics::GraphicsObject::surface()
{
    return surface_;
};

vk::Device fi::graphics::GraphicsObject::device()
{
    return device_;
};

vk::PhysicalDevice fi::graphics::GraphicsObject::physical()
{
    return physical_;
};

vk::PipelineCache fi::graphics::GraphicsObject::pipeline_cache()
{
    return pipeline_cache_;
};

vk::Queue fi::graphics::GraphicsObject::queues(QueueType type)
{
    return queues_[type];
};

uint32_t fi::graphics::GraphicsObject::queue_indices(QueueType type)
{
    return queue_indices_[type];
};

vma::Allocator fi::graphics::GraphicsObject::allocator()
{
    return allocator_;
};

GLFWwindow* fi::graphics::GraphicsObject::window()
{
    return window_;
};

vk::Fence fi::graphics::create_vk_fence(vk::Device device, bool signal)
{
    vk::FenceCreateInfo create_info{};
    if (signal)
    {
        create_info.flags = vk::FenceCreateFlagBits::eSignaled;
    }
    return device.createFence(create_info);
}

vk::Semaphore fi::graphics::create_vk_semaphore(vk::Device device)
{
    vk::SemaphoreCreateInfo create_info{};
    return device.createSemaphore(create_info);
}

vk::Event fi::graphics::create_vk_event(vk::Device device, bool host_event)
{
    vk::EventCreateInfo create_info{.flags = vk::EventCreateFlagBits::eDeviceOnly};
    if (host_event)
    {
        create_info.flags = {};
    }
    return device.createEvent(create_info);
}

fi::graphics::Fence::Fence(bool signal)
    : vk::Fence(create_vk_fence(device(), signal))
{
}

fi::graphics::Fence::~Fence()
{
    device().destroyFence(*this);
}

fi::graphics::Semaphore::Semaphore()
    : vk::Semaphore(create_vk_semaphore(device()))
{
}

fi::graphics::Semaphore::~Semaphore()
{
    device().destroySemaphore(*this);
}

vk::SemaphoreSubmitInfo fi::graphics::Semaphore::submit_info(vk::PipelineStageFlags2 stage)
{
    vk::SemaphoreSubmitInfo submit{};
    submit.setSemaphore(*this);
    submit.stageMask = stage;
    return submit;
}

fi::graphics::Event::Event(bool host_event)
    : vk::Event(create_vk_event(device(), host_event))
{
    std::cout << 1;
}

fi::graphics::Event::~Event()
{
    std::cout << 2;
    device().destroyEvent(*this);
}

fi::graphics::CpuClock::CpuClock()
    : init_(std::chrono::high_resolution_clock::now()),
      begin_(init_)
{
}

fi::graphics::CpuClock::TimePoint fi::graphics::CpuClock::get_elapsed()
{
    return {std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - init_)};
}

fi::graphics::CpuClock::TimePoint fi::graphics::CpuClock::get_delta()
{
    return {std::chrono::duration_cast<std::chrono::milliseconds>(end_ - begin_)};
};

void fi::graphics::CpuClock::start()
{
    begin_ = std::chrono::high_resolution_clock::now();
};

void fi::graphics::CpuClock::reset()
{
    end_ = std::chrono::high_resolution_clock::now();
};

fi::graphics::CpuClock::TimePoint::TimePoint(
    const std::chrono::duration<size_t, std::chrono::milliseconds::period>& duration)
    : duration_{duration}
{
}
