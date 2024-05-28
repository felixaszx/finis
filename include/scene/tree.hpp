#ifndef SCENE_TREE_HPP
#define SCENE_TREE_HPP

#include <iostream>
#include <string>
#include <functional>
#include <queue>
#include <set>

#include "glms.hpp"
#include "extensions/defines.h"

class SceneNode;
struct SceneManager
{
    friend class SceneNode;

  private:
    void* shared_info_ = nullptr;
    const ObjectDetails* details_ = nullptr;
    SceneManagerStates states_ = {};

  public:
    SceneManager(const SceneManagerStates& states, const ObjectDetails* details = nullptr);
    ~SceneManager();
};

class SceneNode
{
  private:
    SceneNode* parent_ = nullptr;
    SceneManager* manager_ = nullptr;
    std::set<SceneNode*> children_ = {};

    glm::vec<3, GlmFloat> position_ = {0, 0, 0}; // always relative
    glm::vec<3, GlmFloat> rotation_ = {0, 0, 0}; // pitch, yaw, roll, saprate from parent
    glm::vec<3, GlmFloat> scale_ = {1, 1, 1};

  public:
    inline static const glm::vec<3, GlmFloat> DEFAULT_FRONT_ = {0, 0, -1};
    inline static const glm::vec<3, GlmFloat> UP_ = {0, 1, 0};

    std::string name_ = "";

    SceneNode(SceneManager& manager, SceneNode* parent = nullptr);
    ~SceneNode();

    SceneNode* get_parent();
    WorldTransform get_world_transform();
    glm::mat<4, 4, GlmFloat> get_world_transform_matrix();
    [[nodiscard]] const glm::vec<3, GlmFloat>& get_position();
    [[nodiscard]] const glm::vec<3, GlmFloat>& get_rotation();
    [[nodiscard]] const glm::vec<3, GlmFloat>& get_scale();
    [[nodiscard]] const std::set<SceneNode*>& get_children();

    [[nodiscard]] const glm::vec<3, GlmFloat> get_front();
    [[nodiscard]] const glm::vec<3, GlmFloat> get_right();
    [[nodiscard]] const glm::vec<3, GlmFloat> get_world_position();
    [[nodiscard]] const glm::vec<3, GlmFloat> get_world_scale();

    void translate(const glm::vec<3, GlmFloat>& destination);
    void move(const glm::vec<3, GlmFloat>& displacement);
    void rotate(const glm::vec<3, GlmFloat>& euler_angles); // in degrees
    void scale(const glm::vec<3, GlmFloat>& scale);
    void set_parent(SceneNode* parent);
    void add_children(const std::set<SceneNode*>& nodes);
    void traverse_breath_first(const std::function<void(SceneNode&)>& call_back = {});
};

#endif // SCENE_TREE_HPP
