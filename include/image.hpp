#ifndef INCLUDE_IMAGE_HPP
#define INCLUDE_IMAGE_HPP

#include "vk_base.hpp"
#include "ext_defines.h"

/**
 * @brief
 *
 * @tparam TypeID: each TypeID share same extension functions
 */
template <uint32_t TypeID>
class Image : public VkObject, //
              public vk::Image
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
          vma::AllocationCreateInfo& alloc_info,  //
          vma::Pool pool = nullptr);
    ~Image();
};

#endif // INCLUDE_IMAGE_HPP
