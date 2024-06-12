#include "scene/scene.hpp"

glm::mat4 WorldObject::update_transform()
{
    glm::mat4 translation(1.0f);
    glm::mat4 rotation(1.0f);
    glm::mat4 scale(1.0f);

    front_ = glm::rotate(rotation_, {1, 0, 0});
    translation = glm::translate(position_);
    rotation = glm::toMat4(rotation_);
    scale = glm::scale(scale_);

    return translation * rotation * scale;
}

WorldObjectHandle World::add_object()
{
    auto handle = registry_.create();
    registry_.emplace<WorldObject>(handle);
    return handle;
}

WorldObject& World::get_object(WorldObjectHandle handle)
{
    return registry_.get<WorldObject>(handle);
}
