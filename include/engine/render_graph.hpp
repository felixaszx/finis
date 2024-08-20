#ifndef ENGINE_RENDER_GRAPH_HPP
#define ENGINE_RENDER_GRAPH_HPP

#include <set>

#include "tools.hpp"
#include "graphics/graphics.hpp"

namespace fi
{
    class RenderGraph : private GraphicsObject
    {
      public:
        using ResIdx = uint32_t;
        using PassIdx = uint32_t;

        struct SyncedRes
        {
            friend RenderGraph;

          public:
            enum Access
            {
                READ,
                WRITE,
                PASS
            };

          protected:
            ResIdx idx_ = -1;

          public:
            std::vector<PassIdx> history_{};
            std::vector<Access> access{};
        };

        struct Buffer : public SyncedRes, //
                        public vk::Buffer,
                        public vma::Allocation
        {
            vk::DeviceSize offset_;
            vk::DeviceSize range_;
        };

        struct Image : public SyncedRes, //
                       public vk::Image,
                       public vma::Allocation
        {
            vk::ImageUsageFlags usages_{};
            vk::ImageSubresource sub_res_{};
            std::vector<vk::ImageView> views_{};

            std::vector<PassIdx> cleared_pass_{};
            std::vector<vk::ImageLayout> layouts_{};
            std::vector<vk::ImageSubresourceRange> ranges_{};
        };

        struct Pass
        {
            friend RenderGraph;

          protected:
            PassIdx idx_ = -1;
            RenderGraph* rg_ = nullptr;

            Image& add_read_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            Image& add_write_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);

          public:
            std::set<ResIdx> input_{};
            std::set<ResIdx> output_{};
            std::set<ResIdx> passing_{};
            std::set<ResIdx> clearing_{};

            void read_buffer(ResIdx buf_idx);
            void write_buffer(ResIdx buf_idx);
            void read_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            void sample_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            void pass_img(ResIdx img_idx, vk::ImageLayout taget_layout, const vk::ImageSubresourceRange& sub_resources);
        };

        struct ComputePass : public Pass
        {
            std::vector<vk::DescriptorImageInfo> des_img_infos_{};

            void write_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
        };

        struct GraphicsPass : public Pass
        {
            using DepthStencilOp = uint32_t;
            enum DepthStencilOpBits : uint32_t
            {
                DEPTH_WRITE = bit_shift_left(0),
                DEPTH_READ = bit_shift_left(1),
                STENCIL_WRITE = bit_shift_left(2),
                STENCIL_READ = bit_shift_left(3)
            };

            std::vector<vk::DescriptorImageInfo> des_img_infos_{};
            std::vector<vk::Format> color_formats_{};
            vk::Format depth_format_{};
            vk::Format stencil_format_{};

            std::vector<vk::RenderingAttachmentInfo> color_atchm_infos_{};
            vk::RenderingAttachmentInfo depth_atchm_info_{};
            vk::RenderingAttachmentInfo stencil_atchm_info_{};

            void clear_img(ResIdx res_idx);
            void write_color_atchm(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            void set_ds_atchm(ResIdx img_idx, DepthStencilOp operation, const vk::ImageSubresourceRange& sub_resources);
        };

      private:
        std::vector<PassIdx> pass_mapping_{};
        std::vector<ComputePass> computes_{};
        std::vector<GraphicsPass> graphics_{};

        std::vector<ResIdx> res_mapping_{};
        std::vector<Buffer> bufs_{};
        std::vector<Image> imgs_{};

      public:
        ResIdx register_atchm(vk::ImageSubresource sub_res, vk::ImageUsageFlagBits initial_usage = {});
        ResIdx register_atchm(vk::Image image, const std::vector<vk::ImageView>& views);
        ResIdx register_buffer(vk::DeviceSize size);
        ResIdx register_buffer(vk::Buffer buffer, vk::DeviceSize offset = 0, vk::DeviceSize range = VK_WHOLE_SIZE);
        PassIdx register_compute_pass(const std::function<void(ComputePass& pass)>& setup_func //
                                      = [](ComputePass& pass) {});
        PassIdx register_graphics_pass(const std::function<void(GraphicsPass& pass)>& setup_func //
                                       = [](GraphicsPass& pass) {});
        ComputePass& get_compute_pass(PassIdx compute_pass_idx);
        GraphicsPass& get_graphics_pass(PassIdx graphics_pass_idx);
        Buffer& get_buffer_res(ResIdx buf_idx);
        Image& get_image_res(ResIdx img_idx);
        void compile(const vk::Extent2D& render_area_);
        void excute(const std::vector<std::function<void()>>& funcs); // funcs.size() == passes_.size()
    };
}; // namespace fi

#endif // ENGINE_RENDER_GRAPH_HPP