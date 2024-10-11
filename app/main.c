#include <stdio.h>
#include <stdlib.h>

#include "fi_ext.h"
#include "gfx/gfx.h"
#include "renderer/gbuffer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

typedef struct render_thr_arg
{
    vk_ctx* ctx_;
    atomic_bool rendering_;
    atomic_long frame_time_;
} render_thr_arg;

typedef struct gbuffer_renderer_arg
{
    vk_swapchain* sc_;
    VkSemaphore acquired_;
    uint32_t image_idx_;

    gltf_anim* sparta_anim_;
    gltf_skin* sparta_skin_;
    vk_mesh* sparta_mesh_;
    vk_mesh_desc* sparta_mesh_desc_;
    vk_mesh_skin* sparta_mesh_skin_;
    vk_tex_arr* sparta_tex_arr_;
} gbuffer_renderer_arg;

void process_sc(gbuffer_renderer* renderer, T* data)
{
    gbuffer_renderer_arg* args = data;
    vk_swapchain_process(args->sc_, renderer->cmd_pool_, args->acquired_, nullptr, &args->image_idx_);
}

void gbuffer_draw(gbuffer_renderer* renderer, T* data)
{
    gbuffer_renderer_arg* args = data;
    struct
    {
        VkDeviceAddress PRIM_COMBO_ARR;
        VkDeviceAddress DATA;
        VkDeviceAddress NODES;
        VkDeviceAddress SKIN;
        mat4 VIEW_MAT;
        mat4 PROJECTION_MAT;
    } pushed_data = {.PRIM_COMBO_ARR = args->sparta_mesh_->address_ + args->sparta_mesh_->prim_offset_,
                     .DATA = args->sparta_mesh_->address_,
                     .NODES = args->sparta_mesh_desc_->address_,
                     .SKIN = args->sparta_mesh_skin_->address_};
    glm_look((vec3){0, 0, -1}, (vec3){0, 0, 1}, (vec3){0, 1, 0}, pushed_data.VIEW_MAT);
    glm_perspective(45.0f, (float)WIDTH / HEIGHT, 0.1, 100.0f, pushed_data.PROJECTION_MAT);

    vkCmdPushConstants(renderer->main_cmd_, renderer->pl_layouts_[0], VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(pushed_data), &pushed_data);
    vk_mesh_draw_prims(args->sparta_mesh_, renderer->main_cmd_);
}

T* render_thr_func(T* arg)
{
    render_thr_arg* ctx_combo = arg;
    vk_ctx* ctx = ctx_combo->ctx_;
    atomic_bool* rendering = &ctx_combo->rendering_;

    VkCommandPool cmd_pool = nullptr;
    VkCommandPoolCreateInfo pool_cinfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_cinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_cinfo.queueFamilyIndex = ctx->queue_idx_;
    vkCreateCommandPool(ctx->device_, &pool_cinfo, nullptr, &cmd_pool);

    vk_swapchain* sc = new (vk_swapchain, ctx);

    VkSemaphore acquired = {};
    VkSemaphoreCreateInfo sem_cinfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(ctx->device_, &sem_cinfo, nullptr, &acquired);

    gbuffer_renderer_arg gbuffer_arg = {.sc_ = sc, .acquired_ = acquired};

    char* pl_dll_name = get_shared_lib_name("./exts/dlls/gbuffer_pl");
    dll_handle default_pl_dll = dlopen(pl_dll_name, RTLD_NOW);
    ffree(pl_dll_name);
    gbuffer_renderer* gbuffer = new (gbuffer_renderer, ctx, default_pl_dll, (VkExtent3D){WIDTH, HEIGHT, 1});
    gbuffer->cmd_begin_cb_ = process_sc;
    gbuffer->render_cb_ = gbuffer_draw;
    gbuffer->sem_submits_[0] = vk_get_sem_info(acquired, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
    dlclose(default_pl_dll);

    gltf_file* sparta = new (gltf_file, "res/models/sparta.glb");
    gltf_desc* sparta_desc = new (gltf_desc, sparta);
    gltf_anim* sparta_anim = new (gltf_anim, sparta, 0);
    gltf_skin* sparta_skin = new (gltf_skin, sparta, sparta_desc);
    VkDeviceSize* prim_transforms = alloc(VkDeviceSize, sparta_desc->mesh_count_);

    vk_mesh* sparta_mesh = new (vk_mesh, ctx, "sparta", to_mb(10), 20);
    vk_prim** sparta_prims = alloc(vk_prim*, sparta->prim_count_);

    for (size_t i = 0; i < sparta_desc->mesh_count_; i++)
    {
        prim_transforms[i] =
            vk_mesh_add_memory(sparta_mesh, sparta_desc->transform_, sizeof(sparta_desc->transform_[0]));
    }

    for (uint32_t i = 0; i < sparta->prim_count_; i++)
    {
        sparta_prims[i] = vk_mesh_add_prim(sparta_mesh);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_INDEX, //
                                sparta->prims_[i].idx_, sparta->prims_[i].idx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_POSITION, //
                                sparta->prims_[i].position, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_NORMAL, //
                                sparta->prims_[i].normal_, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_TANGENT, //
                                sparta->prims_[i].tangent_, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_TEXCOORD, //
                                sparta->prims_[i].texcoord_, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_COLOR, //
                                sparta->prims_[i].color_, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_JOINTS, //
                                sparta->prims_[i].joint_, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_WEIGHTS, //
                                sparta->prims_[i].weight_, sparta->prims_[i].vtx_count_);
        vk_mesh_add_prim_attrib(sparta_mesh, sparta_prims[i], VK_PRIM_ATTRIB_MATERIAL, //
                                sparta->prims_[i].material_, 1);

        uint32_t transform_idx = sparta_desc->prim_transform_[i] - sparta_desc->transform_;
        sparta_mesh->prims_[i].attrib_counts_[VK_PRIM_ATTRIB_TRANSFORM] = 1;
        sparta_mesh->prims_[i].attrib_address_[VK_PRIM_ATTRIB_TRANSFORM] = prim_transforms[transform_idx];
    }
    vk_mesh_alloc_device_mem(sparta_mesh, cmd_pool);

    vk_mesh_desc* sparta_mesh_desc = new (vk_mesh_desc, ctx, sparta_desc->node_count_);
    memcpy(sparta_mesh_desc->nodes_, sparta_desc->nodes_, sizeof(sparta_desc->nodes_[0]) * sparta_desc->node_count_);
    vk_mesh_desc_alloc_device_mem(sparta_mesh_desc);
    vk_mesh_desc_update(sparta_mesh_desc, GLM_MAT4_IDENTITY);
    vk_mesh_desc_flush(sparta_mesh_desc);

    vk_mesh_skin* sparta_mesh_skin = new (vk_mesh_skin, ctx, sparta_skin->joint_count_);
    memcpy(sparta_mesh_skin->joints_, sparta_skin->joints_,
           sizeof(sparta_skin->joints_[0]) * sparta_skin->joint_count_);
    vk_mesh_skin_alloc_device_mem(sparta_mesh_skin, cmd_pool);

    vk_tex_arr* sparta_tex_arr = new (vk_tex_arr, ctx, sparta->tex_count_, sparta->sampler_count_);
    for (uint32_t i = 0; i < sparta->sampler_count_; i++)
    {
        vk_tex_arr_add_sampler(sparta_tex_arr, sparta->sampler_cinfos_ + i);
    }
    for (uint32_t i = 0; i < sparta->tex_count_; i++)
    {
        VkExtent3D extent = gltf_tex_extent(sparta->texs_ + i);
        VkImageSubresource sub_res = {.arrayLayer = 1, //
                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                      .mipLevel = sparta->texs_[i].levels_};
        vk_tex_arr_add_tex(sparta_tex_arr, cmd_pool, sparta->texs_[i].sampler_idx_, sparta->texs_[i].data_,
                           gltf_tex_size(sparta->texs_ + i), &extent, &sub_res);
    }

    gbuffer_arg.sparta_mesh_ = sparta_mesh;
    gbuffer_arg.sparta_mesh_desc_ = sparta_mesh_desc;
    gbuffer_arg.sparta_anim_ = sparta_anim;
    gbuffer_arg.sparta_skin_ = sparta_skin;
    gbuffer_arg.sparta_mesh_skin_ = sparta_mesh_skin;
    gbuffer_arg.sparta_tex_arr_ = sparta_tex_arr;

    printf("Rendering begin\n");
    while (atomic_load(rendering))
    {
        gbuffer_renderer_render(gbuffer, &gbuffer_arg);

        VkPresentInfoKHR present_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &gbuffer->submitted_;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &sc->swapchain_;
        present_info.pImageIndices = &gbuffer_arg.image_idx_;
        vkQueuePresentKHR(ctx->queue_, &present_info);
    }
    gbuffer_renderer_wait_idle(gbuffer);

    vkDestroySemaphore(ctx->device_, acquired, nullptr);
    vkDestroyCommandPool(ctx->device_, cmd_pool, nullptr);

    ffree(sparta_prims);
    ffree(prim_transforms);
    delete (vk_tex_arr, sparta_tex_arr);
    delete (vk_mesh_skin, sparta_mesh_skin);
    delete (vk_mesh_desc, sparta_mesh_desc);
    delete (vk_mesh, sparta_mesh);
    delete (gltf_skin, sparta_skin);
    delete (gltf_desc, sparta_desc);
    delete (gltf_anim, sparta_anim);
    delete (gltf_file, sparta);
    delete (gbuffer_renderer, gbuffer);
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