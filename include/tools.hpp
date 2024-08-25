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

#include <vulkan/vulkan.hpp>
#include <glms.hpp>

#define bit_shift_left(bits) (1 << bits)
#define TRY_FUNC \
    try          \
    {

#define CATCH_BEGIN(error)         \
    }                              \
    catch (std::exception & error) \
    {
#define CATCH_END }
#define CATCH_FUNC \
    }              \
    catch (std::exception & e) { std::cerr << e.what(); }
#define casts(type, value)  (static_cast<type>(value))
#define castr(type, value)  (reinterpret_cast<type>(value))
#define castc(type, value)  (const_cast<type>(value))
#define castf(type, value)  ((type)(value))
#define sizeof_arr(std_arr) (std_arr.size() * sizeof(std_arr[0]))

namespace fi
{
    inline void begin_cmd(const vk::CommandBuffer& cmd,           //
                          vk::CommandBufferUsageFlags flags = {}, //
                          const vk::CommandBufferInheritanceInfo* inheritance = nullptr)
    {
        vk::CommandBufferBeginInfo begin_info{};
        begin_info.pInheritanceInfo = inheritance;
        begin_info.flags = flags;
        cmd.begin(begin_info);
    }

    inline void dispatch_cmd(const vk::CommandBuffer& cmd, const glm::uvec3& work_group)
    {
        cmd.dispatch(work_group.x, work_group.y, work_group.z);
    }
};

template <typename T, typename Q>
void sset(T& dst, const Q& src)
{
    casts(Q&, dst) = src;
}

template <typename T, typename V, typename... Q>
void sset(T& dst, const V& src_f, const Q&... src_b)
{
    sset(dst, src_f);
    sset(dst, src_b...);
}

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

inline consteval size_t operator""_kb(unsigned long long kb) { return 1024 * kb; }
inline consteval size_t operator""_kb(long double kb) { return 1024 * kb; }
inline consteval size_t operator""_mb(unsigned long long mb) { return 1024_kb * mb; }
inline consteval size_t operator""_mb(long double mb) { return 1024_kb * mb; }
inline consteval size_t operator""_gb(unsigned long long gb) { return 1024_mb * gb; }
inline consteval size_t operator""_gb(long double gb) { return 1024_mb * gb; }

#endif // INCLUDE_TOOLS_HPP