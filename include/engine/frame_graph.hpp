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
        vk::Format format_{};
        vk::ImageViewType type_{};
        vk::ImageSubresourceRange range_{};

        bool cleared_ = false;
        std::map<ImgRefIdx, std::pair<vk::ImageLayout, vk::AccessFlagBits2>> history_;

        operator bool() { return casts(vk::ImageView, *this); }

        static bool is_reading(vk::ImageLayout layout);
        static vk::PipelineStageFlagBits2 get_src_stage(vk::AccessFlagBits2 access);
        static vk::PipelineStageFlagBits2 get_dst_stage(vk::AccessFlagBits2 access);
    };

    struct FramePass
    {
        PassFunc func_ = {};
        vk::DependencyInfo deps_{};
        std::map<PassIdx, std::pair<vk::Event, std::vector<vk::ImageMemoryBarrier2>>> img_barriers_;
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
        ~FrameGraph();

        FrameImage& register_image();
        ImgRefIdx register_image_ref(FrameImageRef& ref_detail);
        PassIdx register_pass();
        void push_cluster_break();

        void set_pass_func(PassIdx pass_idx, const PassFunc& func);
        void read_image(PassIdx pass_idx, ImgRefIdx ref_idx);
        void write_image(PassIdx pass_idx, ImgRefIdx ref_idx);
        void sample_image(PassIdx pass_idx, ImgRefIdx ref_idx);
        void write_color_atchm(PassIdx pass_idx, ImgRefIdx ref_idx);
        void set_depth_stencil(PassIdx pass_idx, ImgRefIdx ref_idx, DepthStencilOpMask operations);

        void reset();
        void compile();
        void excute();
    };
}; // namespace fi

#endif // ENGINE_FRAME_GRAPH_HPP
