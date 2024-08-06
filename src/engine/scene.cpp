#include "engine/scene.hpp"

void fi::SceneObj::add_child(std::shared_ptr<SceneObj>& child)
{
    children_.insert(child);
}

glm::vec3 fi::SceneObj::get_world_pos() const
{
    return transform_[3];
}

void traverse_scene_helper(fi::SceneObj& curr, const glm::mat4& parent_transform,
                           const std::function<void(fi::SceneObj&)>& func)
{
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), curr.scale_);
    auto rotation = glm::mat4(curr.rotation_);
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), curr.position_);
    curr.transform_ = parent_transform * trans * rotation * scale;

    func(curr);
    for (auto& child : curr.children_)
    {
        traverse_scene_helper(*child, curr.transform_, func);
    }
}

void fi::SceneObj::traverse_scene(const std::function<void(SceneObj&)>& func)
{
    traverse_scene_helper(*this, transform_, func);
}

std::shared_ptr<fi::SceneObj> fi::SceneRegistry::register_obj()
{
    std::shared_ptr<fi::SceneObj> new_obj(new fi::SceneObj);
    new_obj->id_ = this->create();
    return new_obj;
}