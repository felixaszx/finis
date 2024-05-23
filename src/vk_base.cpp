#define VMA_IMPLEMENTATION
#include "vk_base.hpp"

#include <array>
#include <iostream>

#define BUILD_YEAR_CH0 ((__DATE__[7] - '0') * 1000)
#define BUILD_YEAR_CH1 ((__DATE__[8] - '0') * 100)
#define BUILD_YEAR_CH2 ((__DATE__[9] - '0') * 10)
#define BUILD_YEAR_CH3 ((__DATE__[10] - '0'))
#define BUILD_YEAR     ((BUILD_YEAR_CH0 + BUILD_YEAR_CH1 + BUILD_YEAR_CH2 + BUILD_YEAR_CH3))

#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')

#define BUILD_MONTH_CH0 ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? 10 : 0)

#define BUILD_MONTH_CH1         \
    ((BUILD_MONTH_IS_JAN)   ? 1 \
     : (BUILD_MONTH_IS_FEB) ? 2 \
     : (BUILD_MONTH_IS_MAR) ? 3 \
     : (BUILD_MONTH_IS_APR) ? 4 \
     : (BUILD_MONTH_IS_MAY) ? 5 \
     : (BUILD_MONTH_IS_JUN) ? 6 \
     : (BUILD_MONTH_IS_JUL) ? 7 \
     : (BUILD_MONTH_IS_AUG) ? 8 \
     : (BUILD_MONTH_IS_SEP) ? 9 \
     : (BUILD_MONTH_IS_OCT) ? 0 \
     : (BUILD_MONTH_IS_NOV) ? 1 \
     : (BUILD_MONTH_IS_DEC) ? 2 \
                            : /* error default */ '?')
#define BUILD_MONTH (BUILD_MONTH_CH0 + BUILD_MONTH_CH1)

#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? ((__DATE__[4] - '0') * 10) : 0)
#define BUILD_DAY_CH1 (__DATE__[5] - '0')
#define BUILD_DAY     (BUILD_DAY_CH0 + BUILD_DAY_CH1)
VkBool32 VKAPI_CALL debug_cb(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << pCallbackData->pMessage << " {}\n\n";

    return VK_FALSE;
}

bool Graphics::ready()
{
    return window_ && allocator_ && device_;
}

Graphics::Graphics(int width, int height, bool debug, const std::string& title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    last_key_ = &keys_[0];
    glfwSetKeyCallback(window_,
                       [](GLFWwindow* window, int key, int scancode, int action, int mods)
                       {
                           last_key_ = &keys_[key];
                           if (key > 0)
                           {
                               keys_[key].prev_.store(keys_[key].curr_);
                               keys_[key].curr_ = static_cast<Action>(action);
                           }
                       });

    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    std::array<const char*, 1> VALIDATION_LAYERS{"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> instance_exts{glfw_exts, glfw_exts + glfw_ext_count};
    if (debug)
    {
        instance_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    vk::ApplicationInfo app_info{};
    app_info.applicationVersion = VK_MAKE_VERSION(BUILD_YEAR, BUILD_DAY, BUILD_MONTH);
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    vk::DebugUtilsMessengerCreateInfoEXT debug_utils_creat_info{};
    debug_utils_creat_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | //
                                             vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debug_utils_creat_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | //
                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |    //
                                         vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debug_utils_creat_info.pfnUserCallback = debug_cb;

    vk::InstanceCreateInfo instance_create_info{};
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.setPEnabledExtensionNames(instance_exts);

    if (debug)
    {
        instance_create_info.setPEnabledLayerNames(VALIDATION_LAYERS);
        instance_create_info.pNext = &debug_utils_creat_info;
    }
    instance_ = vk::createInstance(instance_create_info);

    if (debug)
    {
        auto load_func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
        if (load_func(instance_, (VkDebugUtilsMessengerCreateInfoEXT*)&debug_utils_creat_info, nullptr,
                      (VkDebugUtilsMessengerEXT*)&messenger_) != VK_SUCCESS)
        {

            throw std::runtime_error("Do not create validation layers\n");
        }
    }

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
    feature.tessellationShader = true;
    feature.sampleRateShading = true;
    feature.imageCubeArray = true;
    feature.multiDrawIndirect = true;
    feature.shaderSampledImageArrayDynamicIndexing = true;
    vk::PhysicalDeviceVulkan11Features feature11{};
    feature11.multiview = true;
    feature11.shaderDrawParameters = true;
    vk::PhysicalDeviceVulkan12Features feature12{};
    feature12.runtimeDescriptorArray = true;
    vk::PhysicalDeviceVulkan13Features feature13{};
    feature13.dynamicRendering = true;
    feature13.synchronization2 = true;

    vk::PhysicalDeviceFeatures2 feature2{};
    feature2.setFeatures(feature);
    feature2.pNext = &feature11;
    feature11.pNext = &feature12;
    feature12.pNext = &feature13;

    std::vector<const char*> device_ext_names = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char*> device_layer_names;
    if (debug)
    {
        device_layer_names.push_back("VK_LAYER_KHRONOS_validation");
    }

    vk::DeviceCreateInfo device_create_info{};
    device_create_info.pNext = &feature2;
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
    vma_create_info.vulkanApiVersion = app_info.apiVersion;
    vma_create_info.pVulkanFunctions = &vma_function;
    vma_create_info.instance = instance_;
    vma_create_info.physicalDevice = physical_;
    vma_create_info.device = device_;
    allocator_ = vma::createAllocator(vma_create_info);

    vk::PipelineCacheCreateInfo pc_info{};
    pipeline_cache_ = device_.createPipelineCache(pc_info);

    details_.instance_ = instance_;
    details_.surface_ = surface_;
    details_.messenger_ = messenger_;
    details_.device_ = device_;
    details_.physical_ = physical_;
    details_.pipeline_cache_ = pipeline_cache_;
    for (int i = 0; i < 3; i++)
    {
        details_.queues_[i] = queues_[i];
        details_.queue_indices_[i] = queue_indices_[i];
    }

    details_.allocator_ = allocator_;
    details_.window_ = window_;
}

Graphics::~Graphics()
{
    device_.waitIdle();
    device_.destroyPipelineCache(pipeline_cache_);
    instance_.destroySurfaceKHR(surface_);

    allocator_.destroy();
    device_.destroy();
    if (messenger_)
    {
        auto load_func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
        load_func(instance_, messenger_, nullptr);
    }

    instance_.destroy();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

vk::Instance VkObject::instance()
{
    return VkObject::instance_;
}

vk::Device VkObject::device()
{
    return VkObject::device_;
}

vk::Queue VkObject::queues(QueueType type)
{
    return VkObject::queues_[type];
}

GLFWwindow* VkObject::window()
{
    return VkObject::window_;
}

const ObjectDetails* VkObject::details_ptr()
{
    return &VkObject::details_;
}

vk::SurfaceKHR VkObject::surface()
{
    return VkObject::surface_;
}

vk::PhysicalDevice VkObject::physical()
{
    return VkObject::physical_;
}

uint32_t VkObject::queue_indices(QueueType type)
{
    return VkObject::queue_indices_[type];
}

const KeyCode& VkObject::keys(KEY key) const
{
    return keys_[static_cast<int>(key)];
}

const KeyCode& VkObject::last_key() const
{
    return *last_key_;
}

vk::PipelineCache VkObject::pipeline_cache()
{
    return VkObject::pipeline_cache_;
}

vk::DebugUtilsMessengerEXT VkObject::messenger()
{
    return VkObject::messenger_;
}

vma::Allocator VkObject::allocator()
{
    return VkObject::allocator_;
}

vk::Fence create_vk_fence(vk::Device device, bool signal)
{
    vk::FenceCreateInfo create_info{};
    if (signal)
    {
        create_info.flags = vk::FenceCreateFlagBits::eSignaled;
    }
    return device.createFence(create_info);
}

vk::Semaphore create_vk_semaphore(vk::Device device)
{
    vk::SemaphoreCreateInfo create_info{};
    return device.createSemaphore(create_info);
}

Fence::Fence(bool signal)
    : vk::Fence(create_vk_fence(device(), signal))
{
}

Fence::~Fence()
{
    device().destroyFence(*this);
}

Semaphore::Semaphore()
    : vk::Semaphore(create_vk_semaphore(device()))
{
}

Semaphore::~Semaphore()
{
    device().destroySemaphore(*this);
}

bool KeyCode::short_release() const
{
    bool result = get(Action::PRESS, Action::RELEASE);
    if (result)
    {
        prev_ = Action::RELEASE;
    }
    return result;
}

bool KeyCode::hold_release() const
{
    bool result = get(Action::HOLD, Action::RELEASE);
    if (result)
    {
        prev_ = Action::RELEASE;
    }
    return result;
}
