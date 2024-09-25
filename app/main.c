#include <stdio.h>
#include <stdlib.h>

#include "fi_vk.h"
#include "fi_ext.h"
#include "vk_mesh.h"
#include "vk_desc.h"
#include "vk_pipeline.h"
#include "res_loader.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

typedef struct render_thr_arg
{
    vk_ctx* ctx_;
    atomic_bool rendering_;
    atomic_long frame_time_;
} render_thr_arg;

T* render_thr_func(T* arg)
{
    render_thr_arg* ctx_combo = arg;
    vk_ctx* ctx = ctx_combo->ctx_;
    atomic_bool* rendering = &ctx_combo->rendering_;

    vk_swapchain* sc = new (vk_swapchain, ctx);
    gltf_file* sparta = new (gltf_file, "res/models/sparta.glb");
    gltf_desc* sparta_desc = new (gltf_desc, sparta);

    VkFence frame_fence = {};
    VkFenceCreateInfo fence_cinfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_cinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(ctx->device_, &fence_cinfo, nullptr, &frame_fence);

    VkSemaphore acquired = {};
    VkSemaphore submitted = {};
    VkSemaphoreCreateInfo sem_cinfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &acquired);
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &submitted);
    VkSemaphoreSubmitInfo waits[1] = {vk_get_sem_info(acquired, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)};
    VkSemaphoreSubmitInfo signals[1] = {vk_get_sem_info(submitted, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)};

    VkCommandPool cmd_pool = {};
    VkCommandPoolCreateInfo pool_cinfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, nullptr, &cmd_pool);

    VkCommandBuffer cmd = {};
    VkCommandBufferAllocateInfo cmd_alloc = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmd_alloc.commandPool = cmd_pool;
    cmd_alloc.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device_, &cmd_alloc, &cmd);
    VkCommandBufferSubmitInfo cmd_submits[1] = {};
    cmd_submits[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_submits[0].commandBuffer = cmd;

    vk_mesh* mesh = new (vk_mesh, ctx, "test_mesh", to_mb(10), 100);
    vk_prim* prim = vk_mesh_add_prim(mesh);
    vk_mesh_add_prim_attrib(mesh, prim, INDEX, sparta->prims_[0].idx_, sparta->prims_[0].idx_count_);
    vk_mesh_add_prim_attrib(mesh, prim, POSITION, sparta->prims_[0].position, sparta->prims_[0].vtx_count_);
    vk_mesh_add_prim_attrib(mesh, prim, TEXCOORD, sparta->prims_[0].texcoord_, sparta->prims_[0].vtx_count_);
    vk_mesh_add_prim_attrib(mesh, prim, NORMAL, sparta->prims_[0].normal_, sparta->prims_[0].vtx_count_);
    vk_mesh_add_prim_attrib(mesh, prim, TANGENT, sparta->prims_[0].tangent_, sparta->prims_[0].vtx_count_);
    vk_mesh_alloc_device_mem(mesh, cmd_pool);

    vk_mesh_desc* mesh_desc = new (vk_mesh_desc, ctx, sparta_desc->node_count_);
    memcpy(mesh_desc->nodes_, sparta_desc->nodes_, mesh_desc->node_count_ * sizeof(*mesh_desc->nodes_));
    vk_mesh_desc_alloc_device_mem(mesh_desc);
    vk_mesh_desc_update(mesh_desc, GLM_MAT4_IDENTITY);
    vk_mesh_desc_flush(mesh_desc);

    VkPushConstantRange range = {};
    range.size = 16;
    dll_handle test_pl_dll = dlopen("exts/dlls/test_pl.dll", RTLD_NOW);
    vk_gfx_pl_desc* pl_desc = new (vk_gfx_pl_desc, dlsym(test_pl_dll, "configurator"), dlsym(test_pl_dll, "cleaner"));
    pl_desc->push_range_ = &range;
    pl_desc->push_range_count_ = 1;
    pl_desc->push_range_->stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    VkPipelineLayout pl_layout = {};
    VkPipeline pl = vk_gfx_pl_desc_build(pl_desc, ctx, &pl_layout);
    dlclose(test_pl_dll);

    VkImage atchm = {};
    VmaAllocation atchm_alloc = {};
    VkImageView atchm_view = {};
    VkImageCreateInfo atchm_cinfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    atchm_cinfo.imageType = VK_IMAGE_TYPE_2D;
    atchm_cinfo.arrayLayers = 1;
    atchm_cinfo.mipLevels = 1;
    atchm_cinfo.samples = VK_SAMPLE_COUNT_1_BIT;
    atchm_cinfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    atchm_cinfo.extent = (VkExtent3D){WIDTH, HEIGHT, 1};
    atchm_cinfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateImage(ctx->allocator_, &atchm_cinfo, &alloc_info, //
                   &atchm, &atchm_alloc, nullptr);

    VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = atchm;
    view_info.format = atchm_cinfo.format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.layerCount = atchm_cinfo.arrayLayers;
    view_info.subresourceRange.levelCount = atchm_cinfo.mipLevels;
    vkCreateImageView(ctx->device_, &view_info, nullptr, &atchm_view);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = atchm;
    barrier.subresourceRange.aspectMask = view_info.subresourceRange.aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = atchm_cinfo.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = atchm_cinfo.arrayLayers;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkCommandBufferBeginInfo begin = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &begin);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, //
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkResetFences(ctx->device_, 1, &frame_fence);
    vkQueueSubmit(ctx->queue_, 1, &submit, frame_fence);

    VkRenderingAttachmentInfo atchm_info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    atchm_info.clearValue.color = (VkClearColorValue){0, 0, 0, 1};
    atchm_info.imageLayout = barrier.newLayout;
    atchm_info.imageView = atchm_view;
    atchm_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    atchm_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.pColorAttachments = &atchm_info;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.renderArea.extent.width = WIDTH;
    rendering_info.renderArea.extent.height = HEIGHT;
    rendering_info.layerCount = 1;

    delete (gltf_file, sparta);
    delete (gltf_desc, sparta_desc);

    clock_t start = clock();
    while (atomic_load_explicit(rendering, memory_order_relaxed))
    {
        uint32_t image_idx = -1;

        vkWaitForFences(ctx->device_, 1, &frame_fence, true, UINT64_MAX);
        vk_swapchain_process(sc, cmd_pool, acquired, nullptr, &image_idx);
        vkResetFences(ctx->device_, 1, &frame_fence);
        atomic_store_explicit(&ctx_combo->frame_time_, clock() - start, memory_order_relaxed);
        start = clock();

        struct
        {
            VkDeviceSize prim_address;
            VkDeviceSize data_address;
        } pushed = {};
        pushed.prim_address = mesh->address_ + mesh->prim_offset_;
        pushed.data_address = mesh->address_;

        vkResetCommandBuffer(cmd, 0);
        vkBeginCommandBuffer(cmd, &begin);
        vkCmdBeginRendering(cmd, &rendering_info);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pl);
        VkViewport viewport = {.width = WIDTH, .height = HEIGHT};
        VkRect2D scissor = {.extent = (VkExtent2D){WIDTH, HEIGHT}};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdPushConstants(cmd, pl_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushed), &pushed);
        vk_mesh_draw_prims(mesh, cmd);
        vkCmdEndRendering(cmd);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit.waitSemaphoreInfoCount = 1;
        submit.pWaitSemaphoreInfos = waits;
        submit.signalSemaphoreInfoCount = 1;
        submit.pSignalSemaphoreInfos = signals;
        submit.commandBufferInfoCount = 1;
        submit.pCommandBufferInfos = cmd_submits;
        VkResult result = vkQueueSubmit2(ctx->queue_, 1, &submit, frame_fence);

        VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &submitted;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc->swapchain_;
        present_info.pImageIndices = &image_idx;
        vkQueuePresentKHR(ctx->queue_, &present_info);
    }

    vkWaitForFences(ctx->device_, 1, &frame_fence, true, UINT64_MAX);

    vkDestroyFence(ctx->device_, frame_fence, nullptr);
    vkDestroySemaphore(ctx->device_, acquired, nullptr);
    vkDestroySemaphore(ctx->device_, submitted, nullptr);
    vkDestroyCommandPool(ctx->device_, cmd_pool, nullptr);
    vkDestroyPipeline(ctx->device_, pl, nullptr);
    vkDestroyPipelineLayout(ctx->device_, pl_layout, nullptr);
    vmaDestroyImage(ctx->allocator_, atchm, atchm_alloc);
    vkDestroyImageView(ctx->device_, atchm_view, nullptr);

    delete (vk_gfx_pl_desc, pl_desc);
    delete (vk_mesh, mesh);
    delete (vk_mesh_desc, mesh_desc);
    delete (vk_swapchain, sc);
    return nullptr;
}

int main(int argc, char** argv)
{
    vk_ctx* ctx = new (vk_ctx, WIDTH, HEIGHT, false);
    render_thr_arg render_thr_args = {ctx};
    atomic_init(&render_thr_args.rendering_, true);
    atomic_init(&render_thr_args.frame_time_, 0);

    pthread_t render_thr = {};
    pthread_create(&render_thr, nullptr, render_thr_func, &render_thr_args);

    while (vk_ctx_update(ctx))
    {
        if (glfwGetKey(ctx->win_, GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(ctx->win_, true);
            break;
        }

        if (glfwGetWindowAttrib(ctx->win_, GLFW_ICONIFIED))
        {
            ms_sleep(1);
        }
    }

    atomic_store(&render_thr_args.rendering_, false);
    pthread_join(render_thr, nullptr);

    delete (vk_ctx, ctx);
    return 0;
}