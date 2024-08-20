#ifndef ENGINE_FRAME_GRAPH_HPP
#define ENGINE_FRAME_GRAPH_HPP

#include <map>
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
    using PassIdx = uint32_t;
    using ImgRefIdx = uint32_t;
    using PassFunc = std::function<void(vk::CommandBuffer cmd)>;

    struct FrameImage : public vk::Image, //
                        public vma::Allocation
    {
        vk::Extent3D extent_{};
        vk::Format format_{};
        vk::ImageType type_{};
        vk::ImageUsageFlags usage_{};
        vk::ImageLayout init_layout_{};
        vk::ImageSubresource sub_resources_{};
    };

    struct FrameImageRef : public vk::ImageView
    {
        vk::Image image_ = nullptr;
        vk::ImageViewType type_{};
        vk::ImageSubresourceRange range_{};
        std::map<ImgRefIdx, std::pair<vk::ImageLayout, vk::AccessFlags2>> history_;

       static bool is_writing(vk::ImageLayout layout);
    };

    struct FramePass
    {
        vk::Event event_{};
        vk::DependencyInfo deps_{};
        std::vector<vk::ImageMemoryBarrier2> img_barriers_{};
        PassFunc func_ = {};
    };

    // this frame graph only support dynamic rendering
    struct FrameGraph : private GraphicsObject
    {
      public:
        using DepthStencilOpMask = uint32_t;
        enum DepthStencilOps : uint32_t
        {
            DEPTH_WRITE = bit_shift_left(0),
            DEPTH_READ = bit_shift_left(1),
            STENCIL_WRITE = bit_shift_left(2),
            STENCIL_READ = bit_shift_left(3)
        };

      private:
        bool loaded = false;
        std::vector<FrameImage> imgs_{};
        std::vector<FrameImageRef> img_refs_{};
        std::vector<FramePass> passes_{};

      public:
        FrameImage& register_image();
        ImgRefIdx register_image_ref(FrameImageRef& ref_detail);
        PassIdx register_pass();

        void set_pass_func(PassIdx pass_idx, const PassFunc& func);
        void read_image(PassIdx pass_idx, ImgRefIdx ref_idx);
        void write_image(PassIdx pass_idx, ImgRefIdx ref_idx);
        void pass_image(PassIdx pass_idx, ImgRefIdx ref_idx, vk::ImageLayout layout);
        void sample_image(PassIdx pass_idx, ImgRefIdx ref_idx);
        void write_color_atchm(PassIdx pass_idx, ImgRefIdx ref_idx);
        void set_depth_stencil(PassIdx pass_idx, ImgRefIdx ref_idx, DepthStencilOpMask operations);

        void reset();
        void compile();
        void excute();
    };
}; // namespace fi

#endif // ENGINE_FRAME_GRAPH_HPP
