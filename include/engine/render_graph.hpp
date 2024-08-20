#ifndef ENGINE_RENDER_GRAPH_HPP
#define ENGINE_RENDER_GRAPH_HPP

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
    // this reander graph only support dynamic rendering
    class RenderGraph : private GraphicsObject
    {
      public:
        RenderGraph(const RenderGraph&) = delete;
        RenderGraph(RenderGraph&&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;
        RenderGraph& operator=(RenderGraph&&) = delete;

        using ResIdx = uint32_t;
        using PassIdx = uint32_t;

        struct SyncedRes : public vma::Allocation, //
                           public vk::DeviceMemory
        {
            friend RenderGraph;

          public:
            enum Access
            {
                READ,
                WRITE,
                PASS,
                INITIAL
            };

            enum Type
            {
                BUFFER,
                IMAGE
            };

          protected:
            ResIdx idx_ = -1;

          public:
            vk::DeviceSize allocated_offset_ = 0;
            vk::DeviceSize allocated_size_ = 0;
            std::vector<PassIdx> history_{};
            std::vector<Access> access_{};

            inline operator bool() noexcept { return casts(vma::Allocation, *this); }
            Access find_prev_access(PassIdx this_pass);
            Access find_next_access(PassIdx this_pass);
        };

        struct Buffer : public SyncedRes, //
                        public vk::Buffer
        {
            vk::DeviceSize offset_ = 0;
            vk::DeviceSize range_ = 0;
            vk::BufferUsageFlags usage_{};

            bool presistant_mapping_ = false;
        };

        struct Image : public SyncedRes, //
                       public vk::Image
        {
            vk::Extent3D extent_{};
            vk::ImageType type_{};
            vk::Format format_{};
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

            enum Type
            {
                GRAPHICS,
                COMPUTE
            };

          protected:
            PassIdx idx_ = -1;
            RenderGraph* rg_ = nullptr;

            Image& add_read_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            Image& add_write_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            static void set_image_barrier(vk::ImageMemoryBarrier2& barrier,
                                          vk::ImageLayout old_layout,
                                          vk::ImageLayout new_layout);

          public:
            vk::DependencyInfo dep_info_{};
            std::vector<vk::ImageMemoryBarrier2> wait_imgs_{};

            std::set<PassIdx> waiting_{};
            std::set<ResIdx> input_{};
            std::set<ResIdx> output_{};
            std::set<ResIdx> passing_{};

            void read_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            void sample_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            void pass_img(ResIdx img_idx, vk::ImageLayout taget_layout, const vk::ImageSubresourceRange& sub_resources);
        };

        struct ComputePass : public Pass
        {
            using Setup = std::function<void(ComputePass& pass)>;

            void write_img(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
        };

        struct GraphicsPass : public Pass
        {
            using Setup = std::function<void(GraphicsPass& pass)>;
            using DepthStencilOp = uint32_t;
            enum DepthStencilOpBits : uint32_t
            {
                DEPTH_WRITE = bit_shift_left(0),
                DEPTH_READ = bit_shift_left(1),
                STENCIL_WRITE = bit_shift_left(2),
                STENCIL_READ = bit_shift_left(3)
            };

            void clear_img(ResIdx res_idx);
            void write_color_atchm(ResIdx img_idx, const vk::ImageSubresourceRange& sub_resources);
            void set_ds_atchm(ResIdx img_idx, DepthStencilOp operation, const vk::ImageSubresourceRange& sub_resources);
        };

      private:
        std::vector<PassIdx> pass_mapping_{};
        std::vector<vk::Event> events_{};
        std::vector<Pass::Type> pass_types_{};
        std::vector<ComputePass> computes_{};
        std::vector<GraphicsPass> graphics_{};

        std::vector<ResIdx> res_mapping_{};
        std::vector<SyncedRes::Type> res_types_{};
        std::vector<Buffer> bufs_{};
        std::vector<Image> imgs_{};

        std::vector<vk::ImageMemoryBarrier2> final_transitions_{};
        std::vector<vk::ImageMemoryBarrier2> initial_transitions_{};
        std::vector<std::set<PassIdx>> exc_groups_{};

      public:
        RenderGraph() = default;
        ~RenderGraph();

        ResIdx register_atchm(vk::ImageSubresource sub_res,
                              vk::Extent3D extent,
                              vk::Format format,
                              vk::ImageType type = vk::ImageType::e2D,
                              vk::ImageUsageFlagBits initial_usage = {});
        ResIdx register_atchm(vk::Image image, const std::vector<vk::ImageView>& views);
        ResIdx register_buffer(vk::DeviceSize size, bool presistant_mapping = false);
        ResIdx register_buffer(vk::Buffer buffer,
                               vk::DeviceSize offset = 0,
                               vk::DeviceSize range = VK_WHOLE_SIZE,
                               bool presistant_mapping = false);
        Buffer& get_buffer_res(ResIdx buf_idx);
        Image& get_image_res(ResIdx img_idx);

        PassIdx register_compute_pass(const ComputePass::Setup& setup_func = {});
        PassIdx register_graphics_pass(const GraphicsPass::Setup& setup_func = {});
        void set_compute_pass(PassIdx compute_pass_idx, const ComputePass::Setup& setup_func);
        void set_graphics_pass(PassIdx graphics_pass_idx, const GraphicsPass::Setup& setup_func);

        void allocate_res();
        void reset();
        void compile();
        void excute(vk::CommandBuffer cmd, const std::vector<std::function<void(vk::CommandBuffer cmd)>>& funcs);
    };
}; // namespace fi

#endif // ENGINE_RENDER_GRAPH_HPP