#include "engine/scene.hpp"

fi::SceneResources::SceneResources(const std::filesystem::path& res_path)
{
    std::filesystem::directory_iterator file_iter(res_path);
    for (const auto& file : file_iter)
    {
        res_detail_->add_gltf_file(file.path());
    }
    res_structure_.create_wtih(res_detail_);
    res_skin_.create_wtih(res_detail_, res_structure_);
    for (size_t g = 0; g < res_detail_->gltf_.size(); g++)
    {
        res_anims_.push_back(get_res_animations(res_detail_, res_structure_, g));
    }
    res_detail_->lock_and_load();
}
