#include <iostream>
#include "vk_base.hpp"

int main(int argc, char** argv)
{
    Graphics graphic(1920, 1080, true);
    while (!glfwWindowShouldClose(graphic.window()))
    {
        glfwPollEvents();
    }
    return EXIT_SUCCESS;
}