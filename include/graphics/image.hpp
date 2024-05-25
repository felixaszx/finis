#ifndef INCLUDE_IMAGE_HPP
#define INCLUDE_IMAGE_HPP

#include "vk_base.hpp"

/**
 * @brief
 *
 * @tparam TypeID: each TypeID share same extension functions
 */
template <uint32_t TypeID>
class Image : public vk::Image, //
              private VkObject

{
  private:
    inline static ImageFunctions funcs_ = {};

    vk::DeviceSize size_ = 0;
    vk::DeviceMemory memory_ = nullptr;
    vma::Allocation allocation_ = nullptr;

  public:
    static void load_funcs(const ImageFunctions& funcs);

    Image(const Image&) = delete;
    Image(const vk::ImageCreateInfo& buffer_info, //
          vma::AllocationCreateInfo& alloc_info);
    ~Image();

    [[nodiscard]] bool valid() const;
};

template <uint32_t TypeID>
inline Image<TypeID>::Image(const vk::ImageCreateInfo& image_info, //
                            vma::AllocationCreateInfo& alloc_info)
{
    CreateImageReturn r = funcs_.create_image_(details_ptr(), //
                                               &static_cast<const VkImageCreateInfo&>(image_info),
                                               &static_cast<const VmaAllocationCreateInfo&>(alloc_info));
    static_cast<vk::Image&>(*this) = r.image_;
    memory_ = r.memory_;
    allocation_ = r.alloc_;
    size_ = r.size_;
}

template <uint32_t TypeID>
inline Image<TypeID>::~Image()
{
    if (valid())
    {
        funcs_.destory_image_(details_ptr(), *this, allocation_);
    }
}

template <uint32_t TypeID>
inline bool Image<TypeID>::valid() const
{
    return static_cast<vk::Image>(*this);
}

#endif // INCLUDE_IMAGE_HPP
