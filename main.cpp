#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/buffer.hpp"
#include "graphics/render_mgr.hpp"
#include "graphics/animation_mgr.hpp"

int main(int argc, char** argv)
{
    using namespace fi;

    Graphics g(1920, 1080, true);
    Swapchain sc;
    sc.create();

    TextureMgr texture_mgr;
    PipelineMgr pipeline_mgr;
    RenderMgr render_mgr;
    AnimationMgr animation_mgr;
    gltf::Expected<gltf::GltfDataBuffer> gltf_file({});
    auto sponza = render_mgr.upload_res("res/models/sponza_gltf/sponza.glb", texture_mgr, animation_mgr, gltf_file);
    render_mgr.lock_and_prepared();

    Semaphore next_img;
    Semaphore submit;
    Fence frame_fence;

    ShaderModule vs("res/shaders/vulkan0.vert", vk::ShaderStageFlagBits::eVertex);
    ShaderModule fs("res/shaders/vulkan0.frag", vk::ShaderStageFlagBits::eFragment);
    std::vector<vk::PipelineShaderStageCreateInfo> shader_infos = {vs, fs};

    vk::PushConstantRange push_range{};
    push_range.size = 3 * sizeof(glm::mat4);
    push_range.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::PipelineLayoutCreateInfo p_layout_info{};
    auto set_layouts = render_mgr.texture_set_layouts();
    p_layout_info.setSetLayouts(set_layouts);
    p_layout_info.setPushConstantRanges(push_range);
    vk::PipelineLayout p_layout = pipeline_mgr.build_pipeline_layout(p_layout_info);

    auto vtx_attributes = Renderable::vtx_attributes();
    auto vtx_bindings = Renderable::vtx_bindings();
    vk::PipelineVertexInputStateCreateInfo vtx_state{};
    vtx_state.setVertexAttributeDescriptions(vtx_attributes);
    vtx_state.setVertexBindingDescriptions(vtx_bindings);

    vk::PipelineInputAssemblyStateCreateInfo input_asm{};
    input_asm.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineTessellationStateCreateInfo tesselation_info{};

    vk::PipelineViewportStateCreateInfo view_port{};
    view_port.viewportCount = 1;
    view_port.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.lineWidth = 1;

    vk::PipelineMultisampleStateCreateInfo multi_sample{};
    multi_sample.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo ds_info{};

    vk::PipelineColorBlendStateCreateInfo blend_states{};
    vk::PipelineColorBlendAttachmentState state{};
    state.colorWriteMask = vk::ColorComponentFlagBits::eR | //
                           vk::ColorComponentFlagBits::eG | //
                           vk::ColorComponentFlagBits::eB | //
                           vk::ColorComponentFlagBits::eA;
    blend_states.setAttachments(state);

    std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eScissor, vk::DynamicState::eViewport};
    vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.setDynamicStates(dynamic_states);

    std::vector<vk::Format> formats = {sc.image_format_};
    vk::PipelineRenderingCreateInfo render_info{};
    render_info.setColorAttachmentFormats(formats);

    vk::GraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.layout = p_layout;
    pipeline_info.pNext = &render_info;
    pipeline_info.setStages(shader_infos);
    pipeline_info.pVertexInputState = &vtx_state;
    pipeline_info.pInputAssemblyState = &input_asm;
    pipeline_info.pTessellationState = &tesselation_info;
    pipeline_info.pViewportState = &view_port;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multi_sample;
    pipeline_info.pDepthStencilState = &ds_info;
    pipeline_info.pColorBlendState = &blend_states;
    pipeline_info.pDynamicState = &dynamic_state_info;
    CombinedPipeline pipeline = pipeline_mgr.build_pipeline(pipeline_info);

    vk::RenderingAttachmentInfo atchm_info({}, vk::ImageLayout::eColorAttachmentOptimal, {}, {}, {},
                                           vk::AttachmentLoadOp::eClear, {}, vk::ClearColorValue{0, 0, 0, 1});
    vk::RenderingInfo rendering({}, {{}, {1920, 1080}}, 1, {}, 1, &atchm_info);

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = g.queue_indices(Graphics::GRAPHICS);
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

    vk::CommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.commandBufferCount = 1;
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.level = vk::CommandBufferLevel::ePrimary;
    auto cmds = g.device().allocateCommandBuffers(cmd_alloc);

    struct
    {
        glm::mat4 model = glm::scale(glm::vec3(0.1, 0.1, 0.1));
        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), float(1920) / 1080, 0.1f, 1000.0f);
    } push;

    uint32_t up = 0;
    while (g.update())
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        atchm_info.imageView = sc.views_[img_idx];
        g.device().resetFences(frame_fence);

        if (glfwGetKey(g.window(), GLFW_KEY_L))
        {
            vs.reset();
            fs.reset();
            shader_infos = {vs, fs};
            pipeline = pipeline_mgr.build_pipeline(pipeline_info);
        }

        cmds[0].reset();
        begin_cmd(cmds[0]);
        cmds[0].beginRendering(rendering);
        render_mgr.draw({sponza.first},
                        [&](vk::Buffer vtx_buffer, uint32_t vtx_buffer_binding,
                            const RenderMgr::VtxIdxBufferExtra& offsets, vk::DescriptorSet set)
                        {
                            pipeline_mgr.bind_pipeline(cmds[0], pipeline);
                            pipeline_mgr.bind_descriptor_sets(cmds[0], set);
                            cmds[0].pushConstants(p_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push), &push);
                            cmds[0].setViewport(0, vk::Viewport(0, 0, 1920, 1080, 0, 1));
                            cmds[0].setScissor(0, vk::Rect2D({}, {1920, 1080}));
                            cmds[0].bindVertexBuffers(0, vtx_buffer, {offsets.vtx_offset_});
                            cmds[0].bindIndexBuffer(vtx_buffer, offsets.idx_offset_, vk::IndexType::eUint32);
                            cmds[0].drawIndexedIndirect(vtx_buffer, offsets.draw_call_offset_, sponza.second,
                                                        sizeof(vk::DrawIndexedIndirectCommand));
                        });
        cmds[0].endRendering();
        cmds[0].end();

        std::vector<vk::PipelineStageFlags> waiting_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        std::vector<vk::Semaphore> submit_sems = {submit};
        std::vector<vk::Semaphore> wait_sems = {next_img};
        vk::SubmitInfo submit_info{};
        submit_info.setSignalSemaphores(submit_sems);
        submit_info.setWaitSemaphores(wait_sems);
        submit_info.setWaitDstStageMask(waiting_stages);
        submit_info.setCommandBuffers(cmds);
        g.queues(GraphicsObject::GRAPHICS).submit(submit_info, frame_fence);
        sc.present(submit_sems);
    }
    g.device().waitIdle();
    sc.destory();

    int* a = new int;
    return EXIT_SUCCESS;
}