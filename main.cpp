#include <iostream>

#include "extensions/loader.hpp"
#include "graphics/graphics.hpp"
#include "graphics/swapchain.hpp"
#include "graphics/buffer.hpp"
#include "graphics/render_target.hpp"

int main(int argc, char** argv)
{
    using namespace fi;

    Graphics g(1920, 1080, true);
    Swapchain sc;
    sc.create();

    ImageMgr image_mgr;
    GltfLoader gltf_loader;
    auto scenes = gltf_loader.from_file("res/models/sponza_gltf/sponza.glb", image_mgr);


    while (g.update())
    {
    }

    sc.destory();
    return EXIT_SUCCESS;
}