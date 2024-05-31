#include <iostream>
#include "graphics/vk_base.hpp"
#include "graphics/swapchain.hpp"
#include "scene/scene.hpp"
#include "extensions/loader.hpp"

int main(int argc, char** argv)
{
    Graphics g(1920, 1080, true);
    Swapchain swapchain;
    swapchain.create();

    ExtensionLoader test_ext("exe/test2.dll");
    auto ext = test_ext.load_extension();
    while (g.running())
    {
    }

    swapchain.destory();
    return EXIT_SUCCESS;
}