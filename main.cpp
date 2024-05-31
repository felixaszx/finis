#include <iostream>
#include "graphics/vk_base.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/texture.hpp"
#include "scene/scene.hpp"
#include "extensions/loader.hpp"

int main(int argc, char** argv)
{
    Graphics g(1920, 1080, true);
    Swapchain swapchain;
    swapchain.create();

    TextureMgr texture_mgr;
    Texture tt = texture_mgr.load_texture("res/textures/elysia.png");

    while (g.running())
    {
    }

    swapchain.destory();
    return EXIT_SUCCESS;
}