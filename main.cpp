#include <iostream>
#include "vk_base.hpp"
#include "ext_loader.hpp"
#include "resources.hpp"
#include "swapchain.hpp"

int main(int argc, char** argv)
{
    Graphics g(1920, 1080, true);
    ExtensionLoader test_ext("exe/finis_test.dll");

    Swapchain sc;
    sc.create();

    while (!glfwWindowShouldClose(g.window()))
    {
        glfwPollEvents();
    }

    sc.destory();
    return EXIT_SUCCESS;
}