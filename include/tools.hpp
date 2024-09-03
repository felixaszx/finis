/**
 * @file tools.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-08-15
 *
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 *
 */
#ifndef INCLUDE_TOOLS_HPP
#define INCLUDE_TOOLS_HPP

#include <vector>
#include <string>
#include <memory>
#include <chrono>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "glms.hpp"
#include "task_thread_pool.hpp"
namespace thp = task_thread_pool;

namespace fi::util
{
    template <typename T, typename Q>
    inline constexpr T casts(Q&& value)
    {
        return static_cast<T>(std::forward<Q>(value));
    }

    template <typename T, typename Q>
    inline constexpr T castr(Q&& value)
    {
        return reinterpret_cast<T>(std::forward<Q>(value));
    }

    template <typename T, typename Q>
    inline constexpr T castc(Q&& value)
    {
        return const_cast<T>(std::forward<Q>(value));
    }

    template <typename T, typename Q>
    inline constexpr T castd(Q&& value)
    {
        return dynamic_cast<T>(std::forward<Q>(value));
    }

    template <typename T, typename Q>
    inline constexpr T castf(Q&& value)
    {
        return (T)(std::forward<Q>(value));
    }

    template <typename T>
    inline std::size_t sizeof_arr(T& arr)
    {
        return (arr.size() * sizeof(arr[0]));
    }

    template <typename C>
    void free_stl_container(C& container)
    {
        C().swap(container);
    }

    template <typename Ptr, typename... Param>
    inline constexpr void make_unique2(std::unique_ptr<Ptr>& unique_ptr, Param&&... param)
    {
        unique_ptr.reset(new Ptr(std::forward<Param>(param)...));
    }

    template <typename T>
    inline constexpr void make_unique2(std::unique_ptr<T>& unique_ptr, T* new_obj)
    {
        unique_ptr.reset(new_obj);
    }

    template <typename Ptr, typename... Param>
    inline constexpr void make_shared2(std::shared_ptr<Ptr>& shared_ptr, Param&&... param)
    {
        shared_ptr.reset(new Ptr(std::forward<Param>(param)...));
    }

    template <typename T>
    inline constexpr void make_shared2(std::shared_ptr<T>& shared_ptr, T* new_obj)
    {
        shared_ptr.reset(new_obj);
    }

    namespace literals
    {
        inline consteval size_t operator""_b(unsigned long long b)
        {
            return b;
        }
        inline consteval size_t operator""_kb(unsigned long long kb)
        {
            return 1024_b * kb;
        }
        inline consteval size_t operator""_kb(long double kb)
        {
            return 1024_b * kb;
        }
        inline consteval size_t operator""_mb(unsigned long long mb)
        {
            return 1024_kb * mb;
        }
        inline consteval size_t operator""_mb(long double mb)
        {
            return 1024_kb * mb;
        }
        inline consteval size_t operator""_gb(unsigned long long gb)
        {
            return 1024_mb * gb;
        }
        inline consteval size_t operator""_gb(long double gb)
        {
            return 1024_mb * gb;
        }
    }; // namespace literals
}; // namespace fi::util

#endif // INCLUDE_TOOLS_HPP