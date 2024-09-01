#include "tools.hpp"
#include "graphics/pipeline.hpp"
#include "extensions/cpp_defines.hpp"

static size_t count = 0;

using namespace fi;
class pipeline : public gfx::gfx_pipeline
{
  private:
  public:
    pipeline()
    {
        std::cout << count;
        count++;
    }
    
    ~pipeline() {}

    vk::Pipeline get_pipeline() override {return {};}
    void browse_shader(gfx::shader* shader) override {}
};

EXPORT_EXTENSION(pipeline);