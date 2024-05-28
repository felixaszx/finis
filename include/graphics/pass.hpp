#ifndef INCLUDE_PASS_HPP
#define INCLUDE_PASS_HPP

#include "vk_base.hpp"

class Pass : private VkObject
{
  private:
    PassFunctions funcs_ = {};
    std::vector<vk::Image> images_;
    std::vector<vk::ImageView> image_views_;
    std::vector<vk::RenderingAttachmentInfo> atchm_infos_;

  public:
    PassChain chain_info_ = {};

    Pass(const PassFunctions& funcs);
    ~Pass();

    void setup();
    void render(vk::CommandBuffer cmd);
    void finish();
};

class PassGroup
{
  private:
    std::vector<Pass> passes_ = {};

  public:
    Pass& register_pass(const PassFunctions& funcs);
    Pass* get_pass(uint32_t id);
};

#endif // INCLUDE_PASS_HPP