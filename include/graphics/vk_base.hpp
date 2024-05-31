#ifndef GRAPHICS_GRAPHICS_HPP
#define GRAPHICS_GRAPHICS_HPP

#include <iostream>
#include <format>
#include <vector>
#include <semaphore>
#include <thread>
#include <mutex>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "glms.hpp"
#include "vma/vk_mem_alloc.hpp"
#include "key_code.hpp"
#include "extensions/cpp_defines.hpp"

enum QueueType
{
    GRAPHICS_QUEUE_IDX = 0,
    COMPUTE_QUEUE_IDX = 1,
    TRANSFER_QUEUE_IDX = 2
};

struct Graphics;
enum class Action
{
    RELEASE = GLFW_RELEASE,
    PRESS = GLFW_PRESS,
    HOLD = GLFW_REPEAT
};

enum class ModKey
{
    SHIFT,
    CTRL,
    ALT,
};

class KeyCode
{
    friend Graphics;

  private:
    mutable std::atomic<Action> prev_ = Action::RELEASE;
    mutable std::atomic<Action> curr_ = Action::RELEASE;

  public:
    [[nodiscard]] bool get(Action first, Action second) const { return (prev_ == first) && (curr_ == second); }
    [[nodiscard]] bool holding() const { return get(Action::HOLD, Action::HOLD); }
    [[nodiscard]] bool just_hold() const { return get(Action::PRESS, Action::HOLD); }
    [[nodiscard]] bool short_release() const;
    [[nodiscard]] bool hold_release() const;
    [[nodiscard]] bool operator==(const Action& action) const { return curr_ == action; }
    [[nodiscard]] bool operator>=(const Action& action) const { return curr_ >= action; }
    [[nodiscard]] bool operator<=(const Action& action) const { return curr_ <= action; }
    operator bool() const { return curr_ >= Action::PRESS; } // pressing
};

static const vk::ColorComponentFlags RGBA_COMPONENT = vk::ColorComponentFlagBits::eR |
                                                      vk::ColorComponentFlagBits::eG | //
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

static const vk::ColorComponentFlags RGB_COMPONENT = vk::ColorComponentFlagBits::eR |
                                                     vk::ColorComponentFlagBits::eG | //
                                                     vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

/**
 * @brief Meta object that contain all vulkan handles
 *
 */
class VkObject
{
    friend Graphics;

  private:
    static const uint32_t ALL_REGISTERED_KEY_COUNT = 350;
    inline static std::array<KeyCode, ALL_REGISTERED_KEY_COUNT> keys_{};
    inline static KeyCode* last_key_{};

    inline static vk::Instance instance_{};
    inline static vk::SurfaceKHR surface_{};
    inline static vk::DebugUtilsMessengerEXT messenger_{};

    inline static vk::Device device_{};
    inline static vk::PhysicalDevice physical_{};
    inline static vk::PipelineCache pipeline_cache_{};

    inline static std::array<vk::Queue, 3> queues_{};
    inline static std::array<uint32_t, 3> queue_indices_{};

    inline static vma::Allocator allocator_{};
    inline static GLFWwindow* window_{};

  public:
    static vk::Instance instance();
    static vk::SurfaceKHR surface();
    static vk::DebugUtilsMessengerEXT messenger();
    static vk::Device device();
    static vk::PhysicalDevice physical();
    static vk::PipelineCache pipeline_cache();
    static vk::Queue queues(QueueType type);
    static uint32_t queue_indices(QueueType type);
    static vma::Allocator allocator();
    static GLFWwindow* window();
    [[nodiscard]] const KeyCode& keys(KEY key) const;
    [[nodiscard]] const KeyCode& last_key() const;
};

struct Graphics : public VkObject
{
    Graphics(int width, int height, bool debug = false, const std::string& title = "");
    ~Graphics();

    static bool ready();
    static bool running();
};

class CpuTimer
{
  private:
    std::chrono::system_clock::time_point init_;
    std::chrono::system_clock::time_point begin_;
    std::chrono::system_clock::time_point end_;

  public:
    CpuTimer();
    float since_init_second();
    uint32_t since_init_ms();

    void start();
    void finish();

    float get_duration_second();
    uint32_t get_duration_ms();
};

vk::Fence create_vk_fence(vk::Device device, bool signal);
vk::Semaphore create_vk_semaphore(vk::Device device);

struct Fence : public vk::Fence, //
               private VkObject
{
    Fence(bool signal = true);
    ~Fence();
};

struct Semaphore : public vk::Semaphore, //
                   private VkObject
{
    Semaphore();
    ~Semaphore();
};

#endif // GRAPHICS_GRAPHICS_HPP;