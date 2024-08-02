#include "graphics2/res_uploader.hpp"

void fi::ResDetails::load_mip_maps()
{
    
}

fi::ResDetails::ResDetails(const std::filesystem::path& path)
{
    gltf_loader().SetPreserveImageChannels(false);
    std::string err = "";
    std::string warnning = "";
    bool loaded = false;
    if (path.extension() == "glb")
    {
        loaded = gltf_loader().LoadBinaryFromFile(&model_, &err, &warnning, path.generic_string());
    }
    else if (path.extension() == "gltf")
    {
        loaded = gltf_loader().LoadASCIIFromFile(&model_, &err, &warnning, path.generic_string());
    }

    if (!loaded)
    {
        std::cout << std::format("{}\n{}\n", err, warnning);
        throw std::runtime_error(std::format("Fail to load {}.\n", path.generic_string()));
        return;
    }
}
