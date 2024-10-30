#ifndef PROTO_RENDER_H
#define PROTO_RENDER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "gfx/gfx.h"

    typedef void (*fi_mesh_pkg_func_t)(T* data);
    typedef struct fi_mesh_pkg
    {
        gltf_anim* anim_;
        gltf_skin* skin_;

        vk_mesh* mesh_;
        vk_mesh_desc* mesh_desc_;
        vk_mesh_skin* mesh_skin_;
        vk_tex_arr* tex_arr_;

        fi_mesh_pkg_func_t func_;
    } fi_mesh_pkg;
#define EMPTY_FI_MESH_PKG (fi_mesh_pkg){};

    typedef void (*fi_pass_interupt_func_t)(T* data);
    typedef struct fi_pass_state
    {
        uint32_t signaled_sem_count_;
        VkSemaphoreSubmitInfo* signaled_sems_;

        uint32_t wait_sem_count_;
        VkSemaphoreSubmitInfo* wait_sems_;

        uint32_t interupt_func_count_;
        fi_pass_interupt_func_t* interupt_funcs_;
    } fi_pass_state;
#define EMPTY_FI_PASS_STATE (fi_pass_state){};

    typedef void (*fi_pass_draw_func_t)(fi_mesh_pkg* pkgs);
    typedef void (*fi_pass_setup_func_t)(fi_pass_state* state, vk_ctx* ctx, vk_swapchain* sc);

#ifdef __cplusplus
}
#endif

#endif // PROTO_RENDER_H
