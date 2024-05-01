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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_XYZW_ONLY
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "vma/vk_mem_alloc.hpp"
#include "key_code.hpp"

#define bit_shift_left(bits) (1 << bits)

namespace glms
{
    template <typename T>
    GLM_FUNC_QUALIFIER glm::mat<4, 4, T, glm::defaultp> perspective(T fovy, T aspect, T zNear, T zFar)
    {
        glm::mat<4, 4, T, glm::defaultp> tmp = glm::perspective(fovy, aspect, zNear, zFar);
        tmp[1][1] *= -1;
        return tmp;
    }

    template <typename G>
    void assign_value(G glm_types, float* arr)
    {
        for (int i = 0; i < glm_types.length(); i++)
        {
            glm_types[i] = arr[i];
        }
    }
} // namespace glms

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
class Object
{
    friend Graphics;

  public:
    enum QueueType
    {
        GRAPHICS = 0,
        COMPUTE = 1,
        TRANSFER = 2
    };

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

struct Graphics : public Object
{
    Graphics(int width, int height, bool debug = false, const std::string& title = "");
    ~Graphics();

    static bool ready();
};

vk::Fence create_vk_fence(vk::Device device, bool signal);
vk::Semaphore create_vk_semaphore(vk::Device device);

struct Fence : public vk::Fence, //
               private Object
{
    Fence(bool signal = true);
    ~Fence();
};

struct Semaphore : public vk::Semaphore, //
                   private Object
{
    Semaphore();
    ~Semaphore();
};

#endif // GRAPHICS_GRAPHICS_HPP;