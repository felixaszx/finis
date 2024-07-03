#include "engine/scene.hpp"

fi::SceneObj::~SceneObj()
{
    auto p = parent_.lock();
    for (auto& child : children_)
    {
        child->parent_ = p;
    }

    if (p)
    {
        p->children_.insert(children_.begin(), children_.end());
    }
}

void fi::SceneObj::build_relation(const std::shared_ptr<SceneObj>& parent, const std::shared_ptr<SceneObj>& child)
{
    auto cp = child->parent_.lock();
    if (cp)
    {
        cp->children_.erase(child);
    }

    child->parent_ = parent;
    parent->children_.insert(child);
}

glm::vec3 fi::SceneObj::get_world_pos() const
{
    return transform_ * glm::vec4(0, 0, 0, 1);
}

void traverse_scene_helper(fi::SceneObj& curr, const std::function<void(fi::SceneObj&)>& func)
{
    auto parent = curr.parent_.lock();

    glm::mat4 scale = glm::scale(curr.transform_, curr.scale_);
    auto rotation = glm::mat4(curr.rotation_);
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), curr.position_);
    curr.transform_ = parent //
                          ? parent->transform_ * trans * rotation * scale
                          : trans * rotation * scale;

    func(curr);
    for (auto& child : curr.children_)
    {
        traverse_scene_helper(*child, func);
    }
}

void fi::SceneObj::traverse_scene(const std::function<void(SceneObj&)>& func)
{
    auto parent = parent_.lock();
    glm::mat4 scale = glm::scale(transform_, scale_);
    auto rotation = glm::mat4(rotation_);
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), position_);
    transform_ = parent //
                     ? parent->transform_ * trans * rotation * scale
                     : trans * rotation * scale;
    func(*this);
    for (auto& child : children_)
    {
        traverse_scene_helper(*child, func);
    }
}

std::shared_ptr<fi::SceneObj> fi::SceneRegistry::register_obj()
{
    std::shared_ptr<fi::SceneObj> new_obj(new fi::SceneObj);
    new_obj->id_ = this->create();
    return new_obj;
}