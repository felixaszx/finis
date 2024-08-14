#include <cmath>
#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/res_loader.hpp"
#include "graphics/res_structure.hpp"
#include "fltk/fl_ext.hpp"

#include "test_pipeline.cpp"

int main(int argc, char** argv)
{
    using namespace fi;
    using namespace program;
    using namespace glms::literal;

    fle::DoubleWindow fltk(800, 600, "");
    fltk.end();
    fle::Flow flow(0, 0, 800, 600);
    fltk.add(flow);
    fltk.resizable(flow);

    fltk.show();
    Graphics g(1920, 1080, "finis");
    Swapchain sc;
    sc.create();

    ResDetails test_res;
    test_res.add_gltf_file("res/models/sparta.glb");
    ResStructure test_structure(test_res);
    test_res.lock_and_load();

    std::vector<vk::DescriptorPoolSize> combined_pool_sizes;
    combined_pool_sizes.insert(combined_pool_sizes.end(), //
                               test_res.des_sizes_.begin(), test_res.des_sizes_.end());
    combined_pool_sizes.insert(combined_pool_sizes.end(), //
                               test_structure.des_sizes_.begin(), test_structure.des_sizes_.end());

    vk::DescriptorPoolCreateInfo des_pool_info{{}, 100};
    des_pool_info.setPoolSizes(combined_pool_sizes);
    vk::DescriptorPool des_pool = g.device().createDescriptorPool(des_pool_info);
    test_res.allocate_descriptor(des_pool);
    test_structure.allocate_descriptor(des_pool);

    std::vector<vk::DescriptorSetLayout> set_layouts = {test_res.set_layout_, test_structure.set_layout_};
    vk::PushConstantRange push_range{};
    push_range.size = 3 * sizeof(glm::mat4);
    push_range.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::PipelineLayoutCreateInfo pso_layout_info{};
    pso_layout_info.setSetLayouts(set_layouts);
    pso_layout_info.setPushConstantRanges(push_range);
    pso_layout = g.device().createPipelineLayout(pso_layout_info);
    test_pipeline(g, sc);

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

    vk::RenderingInfo rendering{};
    rendering.setColorAttachments(color_infos);
    rendering.pDepthAttachment = &depth_stencil_info;
    rendering.pStencilAttachment = &depth_stencil_info;
    rendering.layerCount = 1;
    rendering.renderArea = vk::Rect2D{{}, {1920, 1080}};

    struct
    {
        glm::mat4 model = glm::scale(glm::vec3(0.1, 0.1, 0.1));
        glm::mat4 view = {};
        glm::mat4 proj = glms::perspective(glm::radians(45.0f), float(1920) / 1080, 0.1f, 1000.0f);
    } push;

    CpuClock clock;
    float prev_time = 0;

    glm::vec3 camera_pos = {0, 0, 0};
    float pitch = 0;
    float yaw = -90.0f;
    while (true)
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);
        color_infos[2].imageView = sc.views_[img_idx];

        CpuClock::Second curr_time = clock.get_elapsed();
        float delta_time = curr_time - prev_time;
        prev_time = curr_time;
        glm::vec3 camera_front = glm::normalize(glm::vec3{std::cos(yaw) * std::cos(pitch), //
                                                          std::sin(pitch),                 //
                                                          std::sin(yaw) * std::cos(pitch)});
        {
            if (glfwGetKey(g.window(), GLFW_KEY_W))
            {
                camera_pos += camera_front * delta_time;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_S))
            {
                camera_pos -= camera_front * delta_time;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_A))
            {
                camera_pos -= glm::cross(camera_front, {0, 1, 0}) * delta_time;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_D))
            {
                camera_pos += glm::cross(camera_front, {0, 1, 0}) * delta_time;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_SPACE))
            {
                camera_pos.y += delta_time;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_LEFT_SHIFT))
            {
                camera_pos.y -= delta_time;
            }

            float camera_speed = 2;
            if (glfwGetKey(g.window(), GLFW_KEY_UP))
            {
                pitch += delta_time * camera_speed * 30.0_dg;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_DOWN))
            {
                pitch -= delta_time * camera_speed * 30.0_dg;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_LEFT))
            {
                yaw -= delta_time * camera_speed * 30.0_dg;
            }
            if (glfwGetKey(g.window(), GLFW_KEY_RIGHT))
            {
                yaw += delta_time * camera_speed * 30.0_dg;
            }
        }
        push.view = glm::lookAt(camera_pos, camera_pos + camera_front, glm::vec3(0, 1, 0));

        while (fle::Global::check(), glfwGetWindowAttrib(g.window(), GLFW_ICONIFIED))
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
            g.update();
        }
        if (!g.update())
        {
            break;
        }

        if (glfwGetKey(g.window(), GLFW_KEY_P))
        {
            fltk.show();
        }

        cmds[0].reset();
        begin_cmd(cmds[0]);
        cmds[0].beginRendering(rendering);
        cmds[0].bindPipeline(vk::PipelineBindPoint::eGraphics, pso);
        test_res.bind_res(cmds[0], pso_layout, 0);
        test_structure.bind_res(cmds[0], pso_layout, 1);
        cmds[0].pushConstants(pso_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push), &push);
        cmds[0].setViewport(0, vk::Viewport(0, 0, 1920, 1080, 0, 1));
        cmds[0].setScissor(0, vk::Rect2D({}, {1920, 1080}));
        test_res.draw(cmds[0]);
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

    free_test_pipeline(g, sc);
    g.device().destroyPipelineLayout(pso_layout);
    g.device().destroyDescriptorPool(des_pool);
    sc.destory();
    g.device().destroyCommandPool(cmd_pool);

    fltk.hide();
    fle::Global::check();
    return EXIT_SUCCESS;
}