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

#define casts(type, value)  (static_cast<type>(value))
#define castr(type, value)  (reinterpret_cast<type>(value))
#define castc(type, value)  (const_cast<type>(value))
#define castd(type, value)  (dynamic_cast<type>(value))
#define castf(type, value)  ((type)(value))
#define sizeof_arr(std_arr) (std_arr.size() * sizeof(std_arr[0]))

template <typename T>
T max_of_all(const std::vector<T>& datas)
{
    T max = datas[0];
    for (int i = 0; i < datas.size(); i++)
    {
        if (datas[i] > max)
        {
            max = datas[i];
        }
    }

    return max;
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

template <typename Ptr, typename... Param>
inline constexpr void make_shared2(std::shared_ptr<Ptr>& shared_ptr, Param&&... param)
{
    shared_ptr.reset(new Ptr(std::forward<Param>(param)...));
}

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

#endif // INCLUDE_TOOLS_HPP