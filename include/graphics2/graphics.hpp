#ifndef GRAPHICS_GRAPHICS_HPP
#define GRAPHICS_GRAPHICS_HPP

#include <iostream>
#include <format>
#include <vector>
#include <semaphore>
#include <thread>
#include <mutex>
#include <filesystem>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "glms.hpp"

#include "vma/vk_mem_alloc.hpp"
#include "tinygltf/tiny_gltf.h"

namespace fi
{

    namespace gltf
    {
        using namespace tinygltf;
        inline bool contains(int idx)
        {
            return idx != -1;
        }
    }; // namespace gltf

    struct Graphics;
    class GraphicsObject
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
        inline static vk::Instance instance_{};
        inline static vk::SurfaceKHR surface_{};
        inline static vk::Device device_{};
        inline static vk::PhysicalDevice physical_{};
        inline static vk::PipelineCache pipeline_cache_{};

        inline static std::array<vk::Queue, 3> queues_{};
        inline static std::array<uint32_t, 3> queue_indices_{};
        inline static vk::CommandPool one_time_submit_pool_{};

        inline static vma::Allocator allocator_{};
        inline static GLFWwindow* window_{};

        inline static gltf::TinyGLTF gltf_loader_{};

      public:
        static vk::Instance instance();
        static vk::SurfaceKHR surface();
        static vk::Device device();
        static vk::PhysicalDevice physical();
        static vk::PipelineCache pipeline_cache();
        static vk::Queue queues(QueueType type);
        static uint32_t queue_indices(QueueType type);
        static vma::Allocator allocator();
        static GLFWwindow* window();
        static vk::CommandBuffer one_time_submit_cmd();
        static void submit_one_time_cmd(vk::CommandBuffer cmd);
        static gltf::TinyGLTF& gltf_loader();
    };

    struct Graphics : public GraphicsObject
    {
        Graphics(int width, int height, const std::string& title = "");
        ~Graphics();

        static bool update();
    };

    vk::Fence create_vk_fence(vk::Device device, bool signal);
    vk::Semaphore create_vk_semaphore(vk::Device device);

    struct Fence : public vk::Fence, //
                   private GraphicsObject
    {
        Fence(bool signal = true);
        ~Fence();
    };

    struct Semaphore : public vk::Semaphore, //
                       private GraphicsObject
    {
        Semaphore();
        ~Semaphore();
    };
}; // namespace fi

#endif // GRAPHICS_GRAPHICS_HPP
