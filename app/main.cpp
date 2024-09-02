#include <cmath>
#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/prims.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/shader.hpp"
#include "graphics/swapchain.hpp"
#include "resources/gltf_file.hpp"

int main(int argc, char** argv)
{
    using namespace fi;
    using namespace glms::literal;
    using namespace std::chrono_literals;

    const uint32_t WIN_WIDTH = 1920;
    const uint32_t WIN_HEIGHT = 1080;

    thp::task_thread_pool thread_pool;

    std::vector<std::future<void>> futs;
    res::gltf_file sparta("res/models/sparta.glb", &futs, &thread_pool);
    std::for_each(futs.begin(), futs.end(), [](std::future<void>& fut) { fut.wait(); });

    gfx::context g(WIN_WIDTH, WIN_HEIGHT, "finis");
    gfx::swapchain sc;
    sc.create();

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = g.queue_indices(gfx::context::GRAPHICS);
    vk::CommandPool cmd_pool = g.device().createCommandPool(pool_info);

    gfx::shader pipeline_shader("res/shaders/test.slang");
    ext::loader pl_loader("exe/pipelines.dll");
    auto pl0 = pl_loader.load_unique<gfx::gfx_pipeline>();

    gfx::primitives prims(20_mb, 2000);
    prims.generate_staging_buffer(10_kb);
    prims.add_primitives({sparta.meshes_[1].draw_calls_[0]});
    prims.add_attribute_data(cmd_pool, gfx::prim_info::INDEX, sparta.meshes_[1].prims_[0].idxs_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::POSITON, sparta.meshes_[1].prims_[0].positions_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::NORMAL, sparta.meshes_[1].prims_[0].normals_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::TANGENT, sparta.meshes_[1].prims_[0].tangents_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::TEXCOORD, sparta.meshes_[1].prims_[0].texcoords_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::COLOR, sparta.meshes_[1].prims_[0].colors_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::JOINTS, sparta.meshes_[1].prims_[0].joints_);
    prims.add_attribute_data(cmd_pool, gfx::prim_info::WEIGHTS, sparta.meshes_[1].prims_[0].weights_);
    prims.reload_draw_calls(cmd_pool);

    vk::CommandBufferAllocateInfo cmd_alloc{};
    cmd_alloc.commandBufferCount = 1;
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.level = vk::CommandBufferLevel::ePrimary;
    auto cmds = g.device().allocateCommandBuffers(cmd_alloc);

    gfx::cpu_clock clock;
    gfx::semaphore next_img;
    gfx::semaphore submit;
    gfx::fence frame_fence;
    while (true)
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);

        gfx::cpu_clock::time_pt curr_time = clock.get_elapsed();

        while (glfwGetWindowAttrib(g.window(), GLFW_ICONIFIED))
        {
            std::this_thread::sleep_for(1ms);
            g.update();
        }
        if (!g.update())
        {
            break;
        }

        cmds[0].reset();
        vk::CommandBufferBeginInfo begin_info{};
        cmds[0].begin(begin_info);
        cmds[0].end();

        vk::CommandBufferSubmitInfo cmd_submit{.commandBuffer = cmds[0]};
        vk::SemaphoreSubmitInfo signal_submit = submit.submit_info(vk::PipelineStageFlagBits2::eBottomOfPipe);
        vk::SemaphoreSubmitInfo waite_submit = next_img.submit_info(vk::PipelineStageFlagBits2::eBottomOfPipe);
        vk::SubmitInfo2 submit2{};
        submit2.setCommandBufferInfos(cmd_submit);
        submit2.setSignalSemaphoreInfos(signal_submit);
        submit2.setWaitSemaphoreInfos(waite_submit);
        g.queues(gfx::context::GRAPHICS).submit2(submit2, frame_fence);
        sc.present({submit});
    }
    g.device().waitIdle();

    sc.destory();
    g.device().destroyCommandPool(cmd_pool);

    return EXIT_SUCCESS;
}