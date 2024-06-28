#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/buffer.hpp"
#include "graphics/render_mgr.hpp"

int main(int argc, char** argv)
{
    using namespace fi;

    Graphics g(1920, 1080, true);
    Swapchain sc;
    sc.create();

    ImageMgr image_mgr;


    while (g.update())
    {
    }

    sc.destory();
    return EXIT_SUCCESS;
}