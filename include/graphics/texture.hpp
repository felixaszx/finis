#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP

#include "graphics.hpp"

namespace fi
{ // forward declarations
    class ImageMgr;

    class Image
    {
      protected:
        vk::Image image_{};
        vk::ImageView image_view_{};

        uint32_t levels_ = 1;
        vk::Extent3D extent_{0, 0, 1};

      public:
        vk::Sampler sampler_{};

        operator vk::Image() const { return image_; }
        operator vk::ImageView() const { return image_view_; }
        operator vk::DescriptorImageInfo() const;
    };

    class ImageStorage : public Image, //
                           private GraphicsObject
    {
        friend ImageMgr;

      private:
        vma::Allocation allocation_{};

      public:
        ImageStorage() = default;
        ~ImageStorage();
    };

    class ImageMgr : private GraphicsObject
    {
      private:
        std::unordered_map<std::filesystem::path, ImageStorage> images_;

      public:
        ~ImageMgr();

        [[nodiscard]] Image load_image(const std::filesystem::path& file_path, bool mip_mapping = true);
        void remove_image(const std::filesystem::path& file_path);
        uint32_t size() { return images_.size(); }
    };
}; // namespace fi

#endif // GRAPHICS_TEXTURE_HPP
