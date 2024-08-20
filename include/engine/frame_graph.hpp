#ifndef ENGINE_FRAME_GRAPH_HPP
#define ENGINE_FRAME_GRAPH_HPP

#include <set>

#include "tools.hpp"
#include "graphics/graphics.hpp"

#define ALL_IMAGE_SUBRESOURCES(aspect)                                   \
    vk::ImageSubresourceRange                                            \
    {                                                                    \
        aspect, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS \
    }

namespace fi
{
    struct FrameImage
    {
    };

    struct FramePass
    {
    };

    // this frame graph only support dynamic rendering
    struct FrameGraph : private GraphicsObject
    {
      private:
      public:
    };
}; // namespace fi

#endif // ENGINE_FRAME_GRAPH_HPP
