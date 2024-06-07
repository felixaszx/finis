#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP

#include <unordered_map>

#include "vk_base.hpp"
#include "tools.hpp"

// forward declarations
class TextureMgr;

class Texture
{
  protected:
    vk::Image image_{};
    vk::ImageView image_view_{};
    vk::Sampler sampler_{};

    uint32_t levels_ = 1;
    vk::Extent3D extent_{0, 0, 1};

  public:
    operator vk::Image() { return image_; }
    operator vk::ImageView() { return image_view_; }
    operator vk::DescriptorImageInfo();
};

class TextureStorage : public Texture, //
                       private VkObject
{
    friend TextureMgr;

  private:
    vma::Allocation allocation_{};

  public:
    TextureStorage() = default;
    ~TextureStorage();
};

class TextureMgr : private VkObject
{
  private:
    uint32_t max_textures_;
    vk::Sampler sampler_{};
    vk::DescriptorPool descriptor_pool_{};
    std::unordered_map<std::string, TextureStorage> textures_;

  public:
    TextureMgr(uint32_t max_textures = 100);
    ~TextureMgr();

    [[nodiscard]] Texture load_texture(const std::string& file_path, bool mip_mapping = true);
    void remove_texture(const std::string& file_path);
};

#endif // GRAPHICS_TEXTURE_HPP
