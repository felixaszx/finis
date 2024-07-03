#ifndef ENGINE_SCENE_HPP
#define ENGINE_SCENE_HPP

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <set>
#include <functional>

#include <entt/entt.hpp>
#include "glms.hpp"
#include "scene_comp.hpp"

namespace fi
{
    struct SceneObj
    {
      public:
        entt::entity id_{};
        std::weak_ptr<SceneObj> parent_{};
        std::unordered_set<std::shared_ptr<SceneObj>> children_{};

        glm::vec3 position_ = {0, 0, 0}; // all relative
        glm::vec3 scale_ = {1, 1, 1};
        glm::quat rotation_{};
        glm::mat4 transform_ = glm::mat4(1.0f);

        SceneObj(const SceneObj&) = delete;
        SceneObj(SceneObj&&) = delete;
        SceneObj& operator=(const SceneObj&) = delete;
        SceneObj& operator=(SceneObj&&) = delete;
        SceneObj() = default;
        ~SceneObj();

        operator entt::entity&() { return id_; }
        static void build_relation(const std::shared_ptr<SceneObj>& parent, const std::shared_ptr<SceneObj>& child);

        [[nodiscard]] glm::vec3 get_world_pos() const;
        void traverse_scene(const std::function<void(SceneObj&)>& func = {});
    };

    class SceneRegistry : public entt::registry
    {
      public:
        std::shared_ptr<SceneObj> register_obj();
    };

}; // namespace fi

#endif // ENGINE_SCENE_HPP
