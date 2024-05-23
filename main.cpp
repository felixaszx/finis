#include <iostream>
#include "vk_base.hpp"
#include "ext_loader.hpp"
#include "resources.hpp"

int main(int argc, char** argv)
{
    Graphics g(1920, 1080, true);
    ExtensionLoader test_ext("exe/finis_test.dll");

    Buffer<0>::load_funcs(test_ext.load_buffer_funcs());
    vk::BufferCreateInfo info;
    info.size = 1024;
    vma::AllocationCreateInfo alloc;
    Buffer<0> b(info, alloc);

    while (!glfwWindowShouldClose(g.window()))
    {
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}