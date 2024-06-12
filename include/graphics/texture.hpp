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
    operator vk::Image() const { return image_; }
    operator vk::ImageView() const { return image_view_; }
    operator vk::DescriptorImageInfo() const;
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
    vk::Sampler sampler_{};
    std::unordered_map<std::string, TextureStorage> textures_;

  public:
    TextureMgr();
    ~TextureMgr();

    [[nodiscard]] Texture load_texture(const std::string& file_path, bool mip_mapping = true);
    void remove_texture(const std::string& file_path);
    [[nodiscard]] const std::unordered_map<std::string, TextureStorage>& textures() const;
    uint32_t size() { return textures_.size(); }
};

#endif // GRAPHICS_TEXTURE_HPP
