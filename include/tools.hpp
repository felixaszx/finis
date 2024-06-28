#ifndef INCLUDE_TOOLS_HPP
#define INCLUDE_TOOLS_HPP

#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.hpp>

#define bit_shift_left(bits) (1 << bits)
#define TRY_FUNC \
    try          \
    {

#define CATCH_BEGIN            \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what();
#define CATCH_END }
#define CATCH_FUNC             \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what(); \
    }
#define casts(type, value)   static_cast<type>(value)
#define castr(type, value)   reinterpret_cast<type>(value)
#define castc(type, value)   const_cast<type>(value)
#define castf(type, value)   (type)(value)
#define sizeof_arr(std_arr) std_arr.size() * sizeof(std_arr[0])

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

inline void begin_cmd(const vk::CommandBuffer& cmd,           //
                      vk::CommandBufferUsageFlags flags = {}, //
                      vk::CommandBufferInheritanceInfo* inheritance = nullptr)
{
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.pInheritanceInfo = inheritance;
    begin_info.flags = flags;
    cmd.begin(begin_info);
}

template <typename C>
void free_container_memory(C& container)
{
    C().swap(container);
}

template <typename Obj, typename... Param>
constexpr void make_unique2(std::unique_ptr<Obj>& unique_ptr, Param... param)
{
    unique_ptr = std::make_unique<Obj>(param...);
}

#endif // INCLUDE_TOOLS_HPP
