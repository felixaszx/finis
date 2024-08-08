#include <cmath>
#include <iostream>

#include "extensions/loader.hpp"
#include "graphics2/graphics.hpp"
#include "graphics2/swapchain.hpp"
#include "graphics2/res_uploader.hpp"
#include "graphics2/scene.hpp"
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

    Semaphore next_img;
    Semaphore submit;
    Fence frame_fence;

    ResDetails test_model("res/models/sparta.glb");
    ResSkinDetails test_skins(test_model);
    ResSceneDetails test_scene(test_model);
    std::vector<ResAnimation> test_animations = load_res_animations(test_model);

    std::vector<vk::DescriptorPoolSize> combinned_sizes;
    combinned_sizes.insert(combinned_sizes.end(), test_model.des_sizes_.begin(), test_model.des_sizes_.end());
    combinned_sizes.insert(combinned_sizes.end(), test_skins.des_sizes_.begin(), test_skins.des_sizes_.end());
    combinned_sizes.insert(combinned_sizes.end(), test_scene.des_sizes_.begin(), test_scene.des_sizes_.end());

    vk::DescriptorPoolCreateInfo des_pool_info{};
    des_pool_info.setPoolSizes(combinned_sizes);
    des_pool_info.maxSets = 100;
    vk::DescriptorPool des_pool = g.device().createDescriptorPool(des_pool_info);
    test_model.allocate_descriptor(des_pool);
    test_skins.allocate_descriptor(des_pool);
    test_scene.allocate_descriptor(des_pool);

    std::vector<vk::DescriptorSetLayout> set_layouts = {test_model.set_layout_, test_skins.set_layout_,
                                                        test_scene.set_layout_};
    vk::PushConstantRange push_range{};
    push_range.size = 3 * sizeof(glm::mat4);
    push_range.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::PipelineLayoutCreateInfo pso_layout_info{};
    pso_layout_info.setSetLayouts(set_layouts);
    pso_layout_info.setPushConstantRanges(push_range);
    pso_layout = g.device().createPipelineLayout(pso_layout_info);
    test_pipeline(g, sc);

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
        glm::mat4 model = {};
        glm::mat4 view = {};
        glm::mat4 proj = glms::perspective(glm::radians(45.0f), float(1920) / 1080, 0.1f, 1000.0f);
    } push;

    vk::RenderingInfo rendering{};
    rendering.setColorAttachments(color_infos);
    rendering.pDepthAttachment = &depth_stencil_info;
    rendering.pStencilAttachment = &depth_stencil_info;
    rendering.layerCount = 1;
    rendering.renderArea = vk::Rect2D{{}, {1920, 1080}};

    CpuTimer frame_time;
    float prev_time = 0;
    float curr_time = frame_time.since_init_second();

    glm::vec3 camera_pos = {0, 0, 0};
    float pitch = 0;
    float yaw = -90.0f;
    while (true)
    {
        auto r = g.device().waitForFences(frame_fence, true, std::numeric_limits<uint64_t>::max());
        uint32_t img_idx = sc.aquire_next_image(next_img);
        g.device().resetFences(frame_fence);
        color_infos[2].imageView = sc.views_[img_idx];

        prev_time = curr_time;
        curr_time = frame_time.since_init_second();
        for (size_t node_idx = 0; node_idx < test_scene.nodes_.size(); node_idx++)
        {
            size_t key_frame_idx = test_animations[0].key_frames_idx_[node_idx];
            if (key_frame_idx != -1)
            {
                ResSceneNode& s_node = test_scene.nodes_[node_idx];
                test_animations[0].key_frames_[key_frame_idx].set_sample_time_stamp(curr_time, s_node.translation_,
                                                                                    s_node.rotation_, s_node.scale_);
            }
        }
        test_scene.update_scene();

        glm::vec3 camera_front = glm::normalize(glm::vec3{std::cos(yaw) * std::cos(pitch), //
                                                          std::sin(pitch),                 //
                                                          std::sin(yaw) * std::cos(pitch)});
        float delta_time = curr_time - prev_time;
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
        test_model.bind(cmds[0], 0, pso_layout, 0);
        test_skins.bind(cmds[0], pso_layout, 1);
        test_scene.bind(cmds[0], pso_layout, 2);
        cmds[0].pushConstants(pso_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push), &push);
        cmds[0].setViewport(0, vk::Viewport(0, 0, 1920, 1080, 0, 1));
        cmds[0].setScissor(0, vk::Rect2D({}, {1920, 1080}));
        test_model.draw(cmds[0]);
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
    g.device().destroyDescriptorPool(des_pool);
    free_test_pipeline(g, sc);
    g.device().destroyPipelineLayout(pso_layout);

    fltk.hide();
    fle::Global::check();
    return EXIT_SUCCESS;
}