#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP

#include "graphics.hpp"

namespace fi
{ // forward declarations
    class TextureMgr;

    class Texture
    {
      protected:
        vk::Image image_{};
        vk::ImageView image_view_{};

        uint32_t levels_ = 1;
        vk::Extent3D extent_{0, 0, 1};

      public:
        std::string name_ = "";
        vk::Sampler sampler_{};

        operator vk::Image() const { return image_; }
        operator vk::ImageView() const { return image_view_; }
        operator vk::DescriptorImageInfo() const;
    };

    class TextureStorage : public Texture, //
                         private GraphicsObject
    {
        friend TextureMgr;

      private:
        vma::Allocation allocation_{};

      public:
        TextureStorage() = default;
        ~TextureStorage();
    };

    class TextureMgr : private GraphicsObject
    {
      private:
        std::unordered_map<std::string, TextureStorage> images_;

      public:
        TextureMgr();
        ~TextureMgr();

        [[nodiscard]] fi::Texture& load_texture(const std::string& name,
                                            const gltf::StaticVector<std::uint8_t>& bytes, //
                                            size_t begin, size_t length, bool mip_mapping = true);
        [[nodiscard]] fi::Texture& get_texture(const std::string& name);
        void remove_image(const std::string& file_path);
        uint32_t size() { return images_.size(); }
    };
}; // namespace fi

#endif // GRAPHICS_TEXTURE_HPP
