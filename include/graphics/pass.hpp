#ifndef INCLUDE_PASS_HPP
#define INCLUDE_PASS_HPP

#include "vk_base.hpp"

class Pass : private VkObject
{
  private:
    PassStates states_ = {};
    std::vector<vk::Image> images_;
    std::vector<vk::ImageView> image_views_;
    std::vector<vk::RenderingAttachmentInfo> atchm_infos_;

  public:
    PassChain chain_info_ = {};

    Pass(const PassStates& funcs);
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
    Pass& register_pass(const PassStates& states);
    Pass* get_pass(uint32_t id);
};

#endif // INCLUDE_PASS_HPP