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
#define CATCH_FUNC             \
    }                          \
    catch (std::exception & e) \
    {                          \
        std::cerr << e.what(); \
    }
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

    template <typename T>
    inline std::string indx_error(const T& arr, size_t idx, const std::string& details = "")
    {
        return std::format("Index {}[{}] out of range{}", details, idx, arr.size());
    }
};

template <typename T>
float get_normalized(T integer)
{
    return integer / (float)std::numeric_limits<T>::max();
}

template <typename T>
float get_normalized(T* integer)
{
    return get_normalized(*integer);
}

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
    unique_ptr = std::make_unique<Ptr>(std::forward<Param>(param)...);
}

template <typename Ptr, typename... Param>
inline constexpr void make_shared2(std::shared_ptr<Ptr>& shared_ptr, Param&&... param)
{
    shared_ptr = std::make_shared<Ptr>(std::forward<Param>(param)...);
}

template <typename T>
struct UniqueObj : public std::unique_ptr<T>
{
    inline constexpr UniqueObj(std::nullptr_t obj)
        : std::unique_ptr<T>(obj)
    {
    }

    template <typename... Param>
    inline constexpr UniqueObj(Param&&... param)
        : std::unique_ptr<T>(new T(std::forward<Param>(param)...))
    {
    }

    template <typename... Param>
    inline constexpr void construct_with(Param&&... param)
    {
        this->reset(new T(std::forward<Param>(param)...));
    }

    operator T&() { return *this->get(); }
    operator const T&() const { return *this->get(); }
};

template <typename T>
struct RefObj
{
    T* ptr_ = nullptr;

    RefObj() = default;
    RefObj(T& target) { reference(target); }

    operator T&() { return *ptr_; }
    operator const T&() const { return *ptr_; }

    void reference(T& target) { ptr_ = &target; }
};

template <typename T>
struct SharedObj : public std::shared_ptr<T>
{
    struct Ref : public std::weak_ptr<T>
    {
        inline constexpr Ref(const SharedObj<T>& obj)
            : std::weak_ptr<T>(obj)
        {
        }

        T* test()
        {
            if (this->expired())
            {
                throw std::runtime_error("Reference to SharedObj expired\n");
            }
            else
            {
                *this->lock().get();
            }
        }

        const T* test() const
        {
            if (this->expired())
            {
                throw std::runtime_error("Reference to SharedObj expired\n");
            }
            else
            {
                *this->lock().get();
            }
        }

        operator T&() { return *test(); }
        operator const T&() const { return *test(); }
    };

    inline constexpr SharedObj(std::nullptr_t obj)
        : std::shared_ptr<T>(obj)
    {
    }

    template <typename... Param>
    inline constexpr SharedObj(Param&&... param)
        : std::shared_ptr<T>(new T(std::forward<Param>(param)...))
    {
    }

    template <typename... Param>
    inline constexpr void construct_with(Param&&... param)
    {
        this->reset(new T(std::forward<Param>(param)...));
    }

    operator T&() { return *this->get(); }
    operator const T&() const { return *this->get(); }

    SharedObj<T>::Ref reference() { return {*this}; }
};

#endif // INCLUDE_TOOLS_HPP