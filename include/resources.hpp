#ifndef INCLUDE_RESOURCES_HPP
#define INCLUDE_RESOURCES_HPP

#include "vk_base.hpp"

class Buffer : public VkObject, //
               public vk::Buffer
{
  private:
    vk::DeviceSize size_ = 0;
    vk::DeviceMemory memory_ = nullptr;
    vma::Allocation allocation_ = nullptr;
    void* mapping_ = nullptr;

  public:
    Buffer(const vk::BufferCreateInfo& buffer_info, //
           vma::AllocationCreateInfo& alloc_info,   //
           vma::Pool pool = nullptr);
    ~Buffer();

    void map_memory();
    void unmap_memory();
    void copy_from(void* memory);
    void flush_cache(void* memory, vk::DeviceSize offset = 0, vk::DeviceSize size = 0);
    void invilidate_cache(vk::DeviceSize offset = 0, vk::DeviceSize size = 0);
    [[nodiscard]] bool valid() const;
    [[nodiscard]] void* mapping() const;
    [[nodiscard]] vk::DeviceSize size() const;
    [[nodiscard]] vk::DeviceMemory memory() const;
    [[nodiscard]] vma::Allocation allocation() const;
};

class Image : public VkObject, //
              public vk::Image
{
  private:
    vk::DeviceMemory memory_ = nullptr;
    vma::Allocation allocation_ = nullptr;

  public:
    Image(const vk::ImageCreateInfo& image_info, //
          vma::AllocationCreateInfo& alloc_info, //
          vma::Pool pool = nullptr);
    ~Image();

    [[nodiscard]] bool valid() const;
};

#endif // INCLUDE_RESOURCES_HPP
