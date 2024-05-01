#include <iostream>
#include "vk_base.hpp"

int main(int argc, char** argv)
{
    Graphics graphic(1920, 1080, true);
    while (!glfwWindowShouldClose(graphic.window()))
    {
        glfwPollEvents();
        if (graphic.keys(KEY::ESCAPE).short_release())
        {
            std::cout << "Terminated by ESC\n";
            glfwSetWindowShouldClose(graphic.window(), true);
        }
    }
    return EXIT_SUCCESS;
}