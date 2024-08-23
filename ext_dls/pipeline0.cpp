#include <iostream>

#include "extensions/cpp_defines.hpp"
#include "engine/pipeline.hpp"

class Pipeline0 : public fi::GraphicsPipelineBase
{
  private:
    std::vector<vk::Format> formats_{};

  public:
    Pipeline0() { std::cout << 1; }
    ~Pipeline0() { std::cout << 2; }
};

EXPORT_EXTENSION(Pipeline0, 0);