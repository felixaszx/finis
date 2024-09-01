/**
 * @file graphics.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
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
#include <future>
#include <queue>
#include <cstdint>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#define VMA_LEAK_LOG_FORMAT(format, ...) \
    do                                   \
    {                                    \
        printf((format), __VA_ARGS__);   \
        printf("\n");                    \
    }                                    \
    while (false)
#include "vma/vk_mem_alloc.hpp"

#include "bs_th_pool/BS_thread_pool.hpp"
#include "glms.hpp"

namespace fi::gfx
{
    namespace bst = BS;

    struct context;
    class graphcis_obj
    {
        friend context;

      public:
        enum queue_type
        {
            GRAPHICS = 0,
            COMPUTE = 1,
            TRANSFER = 2
        };

        inline static const uint32_t SUB_GROUP_SIZE_ = 64;

      private:
        inline static vk::Instance instance_{};
        inline static vk::SurfaceKHR surface_{};
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
        static vk::Device device();
        static vk::PhysicalDevice physical();
        static vk::PipelineCache pipeline_cache();
        static vk::Queue queues(queue_type type = GRAPHICS);
        static uint32_t queue_indices(queue_type type = GRAPHICS);
        static vma::Allocator allocator();
        static GLFWwindow* window();
    };

    struct context : public graphcis_obj
    {
        context(int width, int height, const std::string& title = "");
        ~context();

        static bool update();
    };

    vk::Fence create_vk_fence(vk::Device device, bool signal);
    vk::Semaphore create_vk_semaphore(vk::Device device);
    vk::Event create_vk_event(vk::Device device, bool host_event = false);

    struct Fence : public vk::Fence, //
                   private graphcis_obj
    {
        Fence(bool signal = true);
        ~Fence();
    };

    struct Semaphore : public vk::Semaphore, //
                       private graphcis_obj
    {
        Semaphore();
        ~Semaphore();

        vk::SemaphoreSubmitInfo submit_info(vk::PipelineStageFlags2 stage);
    };

    struct Event : public vk::Event, //
                   private graphcis_obj
    {
        Event(bool host_event = false);
        ~Event();
    };

    class cpu_clock
    {
      public:
        using Second = float;
        using DoubleSecond = double;
        using MilliSecond = size_t;
        using SmallMilliSecond = uint32_t;

        struct time_pt
        {
            std::chrono::duration<size_t, std::chrono::milliseconds::period> duration_{};

            time_pt(const std::chrono::duration<size_t, std::chrono::milliseconds::period>& duration);

            inline operator Second() { return duration_.count() / (Second)1000; };
            inline operator DoubleSecond() { return duration_.count() / (DoubleSecond)1000; };
            inline operator MilliSecond() { return duration_.count(); };
            inline operator SmallMilliSecond() { return duration_.count(); };
        };

      private:
        std::chrono::system_clock::time_point init_;
        std::chrono::system_clock::time_point begin_;
        std::chrono::system_clock::time_point end_;

      public:
        cpu_clock();

        time_pt get_elapsed();
        time_pt get_delta();
        void start();
        void reset();
    };
}; // namespace fi::gfx

#endif // GRAPHICS_GRAPHICS_HPP
