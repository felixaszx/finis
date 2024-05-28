#include "scene/tree.hpp"

SceneNode::~SceneNode()
{
    for (SceneNode* child : children_)
    {
        child->parent_ = parent_;
    }

    if (parent_)
    {
        parent_->children_.insert(children_.begin(), children_.end());
    }
}

SceneNode* SceneNode::get_parent()
{
    return parent_;
}

WorldTransform SceneNode::get_world_transform()
{
    glm::vec<3, GlmFloat> world_position = position_;
    glm::vec<3, GlmFloat> world_scale = scale_;

    if (parent_)
    {
        world_position += parent_->position_;
        world_scale *= parent_->scale_;
    }

    glm::mat4 rotation = glm::eulerAngleX(rotation_.x);
    rotation *= glm::eulerAngleY(rotation_.y);
    rotation *= glm::eulerAngleZ(rotation_.z);

    glm::vec<3, GlmFloat> front = DEFAULT_FRONT_;
    front = glm::mat3(rotation) * front;

    glm::vec<3, GlmFloat> right = {};
    right = glm::normalize(glm::cross(front, UP_));

    WorldTransform transform;
    for (int i = 0; i < 3; i++)
    {
        transform.position_[i] = world_position[i];
        transform.rotation_[i] = world_position[i];
        transform.scale_[i] = world_scale[i];
        transform.front_[i] = front[i];
        transform.right_[i] = right[i];
        transform.up_[i] = UP_[i];
    }
    return transform;
}

glm::mat<4, 4, GlmFloat> SceneNode::get_world_transform_matrix()
{
    glm::vec<3, GlmFloat> world_position = position_;
    glm::vec<3, GlmFloat> world_scale = scale_;

    if (parent_)
    {
        world_position += parent_->position_;
        world_scale *= parent_->scale_;
    }

    glm::mat4 rotation = glm::eulerAngleX(rotation_.x);
    rotation *= glm::eulerAngleY(rotation_.y);
    rotation *= glm::eulerAngleZ(rotation_.z);

    return glm::translate(world_position) * rotation * glm::scale(world_scale);
}

const glm::vec<3, GlmFloat>& SceneNode::get_position()
{
    return position_;
}

const glm::vec<3, GlmFloat> SceneNode::get_world_position()
{
    if (parent_)
    {
        return parent_->position_ + position_;
    }
    return position_;
}

const glm::vec<3, GlmFloat>& SceneNode::get_rotation()
{
    return rotation_;
}

const glm::vec<3, GlmFloat>& SceneNode::get_scale()
{
    return scale_;
}

const glm::vec<3, GlmFloat> SceneNode::get_world_scale()
{
    if (parent_)
    {
        return parent_->scale_ * scale_;
    }
    return scale_;
}

const glm::vec<3, GlmFloat> SceneNode::get_front()
{
    glm::vec<3, GlmFloat> front = DEFAULT_FRONT_;
    front = glm::rotateX(front, rotation_.x);
    front = glm::rotateY(front, rotation_.y);
    front = glm::rotateZ(front, rotation_.z);
    return glm::normalize(front);
}

const glm::vec<3, GlmFloat> SceneNode::get_right()
{
    return glm::normalize(glm::cross(get_front(), UP_));
}

const std::set<SceneNode*>& SceneNode::get_children()
{
    return children_;
}

void SceneNode::translate(const glm::vec<3, GlmFloat>& destination)
{
    if (parent_)
    {
        position_ = destination - parent_->position_;
    }
    else
    {
        position_ = destination;
    }
}

void SceneNode::move(const glm::vec<3, GlmFloat>& displacement)
{
    position_ += displacement;
}

void SceneNode::rotate(const glm::vec<3, GlmFloat>& euler_angles)
{
    rotation_ += euler_angles;
}

void SceneNode::scale(const glm::vec<3, GlmFloat>& scale)
{
    scale_ = scale;
}

void SceneNode::set_parent(SceneNode* parent)
{
    if (parent)
    {
        parent->children_.insert(this);
        parent_ = parent;
        position_ -= parent_->position_;
    }
}

void SceneNode::add_children(const std::set<SceneNode*>& nodes)
{
    children_.insert(nodes.begin(), nodes.end());
}

void SceneNode::traverse_breath_first(const std::function<void(SceneNode&)>& call_back)
{
    std::queue<SceneNode*> queue;
    queue.push(this);

    while (!queue.empty())
    {
        SceneNode* curr = queue.front();
        queue.pop();
        for (SceneNode* node : curr->children_)
        {
            queue.push(node);
        }
        call_back(*curr);
    }
}