#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/buffer.hpp"
#include "graphics/render_mgr.hpp"
#include "graphics/animation_mgr.hpp"
#include "fl_ext.hpp"

#include "g_buffer_pipelines.cpp"

int main(int argc, char** argv)
{
    using namespace fi;

    fle::DoubleWindow fltk(800, 600, "Test window");
    fltk.end();
    fle::Flow flow(0, 0, 800, 600);
    fltk.add(flow);
    fltk.resizable(flow);
    fltk.show();

    Graphics g(1920, 1080, "finis");
    Swapchain sc;
    sc.create();

    TextureMgr texture_mgr;
    PipelineMgr pipeline_mgr;
    RenderMgr render_mgr;
    AnimationMgr animation_mgr;
    gltf::Expected<gltf::GltfDataBuffer> gltf_file({});
    auto sponza = render_mgr.upload_res("res/models/sponza/Sponza.gltf", texture_mgr, animation_mgr, gltf_file);
    render_mgr.lock_and_prepared();

    GBufferPass g_buffer;
    g_buffer(pipeline_mgr, render_mgr);

    vk::RenderingInfo rendering{};
    rendering.setColorAttachments(g_buffer.color_infos_);
    rendering.pDepthAttachment = &g_buffer.depth_stencil_info_;
    rendering.pStencilAttachment = &g_buffer.depth_stencil_info_;
    rendering.layerCount = 1;
    rendering.renderArea = vk::Rect2D{{}, {1920, 1080}};

    Semaphore next_img;
    Semaphore submit;
    Fence frame_fence;

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
        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 proj = glms::perspective(glm::radians(45.0f), float(1920) / 1080, 0.1f, 1000.0f);
    } push;

    while (g.update())
    {
        fle::Global::check();
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);

        while (glfwGetWindowAttrib(g.window(), GLFW_ICONIFIED))
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
            g.update();
        }

        if (glfwGetKey(g.window(), GLFW_KEY_L))
        {
            g_buffer.hot_reload(pipeline_mgr);
        }

        if (glfwGetKey(g.window(), GLFW_KEY_P))
        {
            fltk.show();
        }

        cmds[0].reset();
        begin_cmd(cmds[0]);
        cmds[0].beginRendering(rendering);
        render_mgr.draw({sponza.first},
                        [&](vk::Buffer device_buffer, //
                            uint32_t vtx_buffer_binding,
                            const RenderMgr::VtxIdxBufferExtra& offsets, //
                            vk::Buffer host_buffer,
                            const RenderMgr::HostBufferExtra& host_offsets, //
                            vk::DescriptorSet texture_set)
                        {
                            pipeline_mgr.bind_pipeline(cmds[0], g_buffer.pipeline_);
                            pipeline_mgr.bind_descriptor_sets(cmds[0], texture_set);
                            cmds[0].pushConstants(g_buffer.p_layout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push),
                                                  &push);
                            cmds[0].setViewport(0, vk::Viewport(0, 0, 1920, 1080, 0, 1));
                            cmds[0].setScissor(0, vk::Rect2D({}, {1920, 1080}));
                            cmds[0].bindVertexBuffers(vtx_buffer_binding, device_buffer, {offsets.vtx_offset_});
                            cmds[0].bindIndexBuffer(device_buffer, offsets.idx_offset_, vk::IndexType::eUint32);
                            cmds[0].drawIndexedIndirect(host_buffer, host_offsets.draw_call_offset_, sponza.second,
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
    g.device().destroyCommandPool(cmd_pool);

    fltk.hide();
    fle::Global::check();
    return EXIT_SUCCESS;
}