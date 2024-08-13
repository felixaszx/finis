#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/res_loader.hpp"
#include "graphics/shader.hpp"

namespace program
{
    inline std::vector<vk::Image> color_atchms;
    inline std::vector<vma::Allocation> color_alloc;
    inline std::vector<vk::ImageView> color_views;
    inline std::vector<vk::RenderingAttachmentInfo> color_infos{};

    inline vk::Image depth_stencil_atchm;
    inline vma::Allocation depth_stencil_alloc;
    inline vk::ImageView depth_stencil_view;
    inline vk::RenderingAttachmentInfo depth_stencil_info{};

    inline vk::Pipeline pso;
    inline vk::PipelineLayout pso_layout;
} // namespace program

inline void test_pipeline(fi::Graphics& g, fi::Swapchain& sc)
{
    using namespace fi;
    using namespace program;

    // set up attachments
    vk::ImageCreateInfo atchm_info{{},
                                   vk::ImageType::e2D,
                                   vk::Format::eR32G32B32A32Sfloat,
                                   {1920, 1080, 1},
                                   1,
                                   1,
                                   vk::SampleCountFlagBits::e1,
                                   vk::ImageTiling::eOptimal,
                                   vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
                                   vk::SharingMode::eExclusive};
    vma::AllocationCreateInfo atchm_alloc_info{{}, vma::MemoryUsage::eAutoPreferDevice};
    vk::ImageViewCreateInfo atchm_view_info{{},
                                            {},
                                            vk::ImageViewType::e2D,
                                            vk::Format::eR32G32B32A32Sfloat,
                                            {},
                                            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    color_atchms.resize(4);
    color_alloc.reserve(4);
    color_views.reserve(4);
    for (auto& atchm : color_atchms)
    {
        auto allocated = g.allocator().createImage(atchm_info, atchm_alloc_info);
        atchm = allocated.first;
        color_alloc.push_back(allocated.second);
        atchm_view_info.image = atchm;
        color_views.push_back(g.device().createImageView(atchm_view_info));
    }

    vk::ImageCreateInfo ds_info{{},
                                vk::ImageType::e2D,
                                vk::Format::eD24UnormS8Uint,
                                {1920, 1080, 1},
                                1,
                                1,
                                vk::SampleCountFlagBits::e1,
                                vk::ImageTiling::eOptimal,
                                vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                vk::SharingMode::eExclusive};
    vk::ImageViewCreateInfo ds_view_info{
        {},
        {},
        vk::ImageViewType::e2D,
        vk::Format::eD24UnormS8Uint,
        {},
        {vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1}};
    auto ds_allocated = g.allocator().createImage(ds_info, atchm_alloc_info);
    depth_stencil_atchm = ds_allocated.first;
    depth_stencil_alloc = ds_allocated.second;
    ds_view_info.image = depth_stencil_atchm;
    depth_stencil_view = g.device().createImageView(ds_view_info);

    vk::ImageMemoryBarrier barrier{};
    barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    vk::CommandBuffer cmd = g.one_time_submit_cmd();
    begin_cmd(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    for (int i = 0; i < 4; i++)
    {
        barrier.image = color_atchms[i];
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                            {}, {}, {}, barrier);
    }
    barrier.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    barrier.image = depth_stencil_atchm;
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, //
                        {}, {}, {}, barrier);
    cmd.end();
    g.submit_one_time_cmd(cmd);

    color_infos.resize(4);
    for (int i = 0; i < 4; i++)
    {
        color_infos[i].imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        color_infos[i].loadOp = vk::AttachmentLoadOp::eClear;
        color_infos[i].clearValue = vk::ClearColorValue{0, 0, 0, 1};
        color_infos[i].imageView = color_views[i];
    }
    depth_stencil_info.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depth_stencil_info.loadOp = vk::AttachmentLoadOp::eClear;
    depth_stencil_info.clearValue = vk::ClearDepthStencilValue{{1.0f, 0}};
    depth_stencil_info.imageView = depth_stencil_view;

    // set pso
    ShaderModule vs_{"res/shaders/vulkan0.vert", vk::ShaderStageFlagBits::eVertex};
    ShaderModule fs_{"res/shaders/vulkan0.frag", vk::ShaderStageFlagBits::eFragment};
    std::vector<vk::PipelineShaderStageCreateInfo> shader_infos_ = {vs_, fs_};

    vk::PipelineVertexInputStateCreateInfo vtx_state{};
    vk::PipelineInputAssemblyStateCreateInfo input_asm{};
    vk::PipelineTessellationStateCreateInfo tesselation_info{};
    vk::PipelineViewportStateCreateInfo view_port{};
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    vk::PipelineMultisampleStateCreateInfo multi_sample{};
    vk::PipelineDepthStencilStateCreateInfo pso_ds_info{};
    vk::PipelineColorBlendStateCreateInfo blend_states{};
    vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
    vk::PipelineRenderingCreateInfo render_info{};

    input_asm.topology = vk::PrimitiveTopology::eTriangleList;
    view_port.viewportCount = 1;
    view_port.scissorCount = 1;
    rasterizer.lineWidth = 1;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    multi_sample.rasterizationSamples = vk::SampleCountFlagBits::e1;
    pso_ds_info.depthTestEnable = true;
    pso_ds_info.depthCompareOp = vk::CompareOp::eLess;
    pso_ds_info.depthWriteEnable = true;

    std::vector<vk::VertexInputAttributeDescription> vtx_attributes;
    std::vector<vk::VertexInputBindingDescription> vtx_bindings;

    std::vector<vk::PipelineColorBlendAttachmentState> color_state(4);
    for (int i = 0; i < color_state.size(); i++)
    {
        color_state[i].colorWriteMask = vk::ColorComponentFlagBits::eR | //
                                        vk::ColorComponentFlagBits::eG | //
                                        vk::ColorComponentFlagBits::eB | //
                                        vk::ColorComponentFlagBits::eA;
    }
    blend_states.setAttachments(color_state);

    std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};
    dynamic_state_info.setDynamicStates(dynamic_states);

    std::vector<vk::Format> formats = {atchm_info.format, atchm_info.format, atchm_info.format, atchm_info.format};
    render_info.setColorAttachmentFormats(formats);
    render_info.setDepthAttachmentFormat(ds_info.format);
    render_info.setStencilAttachmentFormat(ds_info.format);

    vk::GraphicsPipelineCreateInfo pso_info{};
    pso_info.layout = pso_layout;
    pso_info.pNext = &render_info;
    pso_info.setStages(shader_infos_);
    pso_info.pVertexInputState = &vtx_state;
    pso_info.pInputAssemblyState = &input_asm;
    pso_info.pTessellationState = &tesselation_info;
    pso_info.pViewportState = &view_port;
    pso_info.pRasterizationState = &rasterizer;
    pso_info.pMultisampleState = &multi_sample;
    pso_info.pDepthStencilState = &pso_ds_info;
    pso_info.pColorBlendState = &blend_states;
    pso_info.pDynamicState = &dynamic_state_info;
    pso = g.device().createGraphicsPipelines(g.pipeline_cache(), pso_info).value[0];
};

inline void free_test_pipeline(fi::Graphics& g, fi::Swapchain& sc)
{
    using namespace fi;
    using namespace program;
    // free resources
    for (size_t i = 0; i < color_atchms.size(); i++)
    {
        g.device().destroyImageView(color_views[i]);
        g.allocator().destroyImage(color_atchms[i], color_alloc[i]);
    }
    g.device().destroyImageView(depth_stencil_view);
    g.allocator().destroyImage(depth_stencil_atchm, depth_stencil_alloc);
    g.device().destroyPipeline(pso);
}