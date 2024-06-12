#ifndef SCENE_SCENE_HPP
#define SCENE_SCENE_HPP

#include <set>
#include <functional>
#include <unordered_map>

#include <entt/entity/registry.hpp>

#include "tools.hpp"
#include "glms.hpp"
#include "extensions/cpp_defines.hpp"

using WorldObjectHandle = entt::entity;

struct WorldObject
{
    std::string name_ = "";
    std::string info_ = "";

    glm::vec3 position_{0, 0, 0};
    glm::vec3 scale_{1, 1, 1};
    glm::vec3 front_{1, 0, 0};
    glm::quat rotation_{};

    WorldObject* parent_ = nullptr;

    glm::mat4 update_transform();
};

struct World
{
    entt::registry registry_{};

    WorldObjectHandle add_object();
    WorldObject& get_object(WorldObjectHandle handle);
};

#endif // SCENE_SCENE_HPP