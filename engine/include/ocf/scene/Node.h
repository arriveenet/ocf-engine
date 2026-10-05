// SPDX-License-Identifier: MIT

#pragma once

#include "ocf/core/Object.h"
#include "ocf/math/Transform.h"

#include <memory>
#include <vector>

namespace ocf {

class Component;

/**
 * @brief Node class represents a node in the scene graph.
 */
class OCF_API Node : public Object {
public:
    Node();
    virtual ~Node();

    [[nodiscard]] Node* createChild();

    void destroyChild(Node* child);

    const std::vector<std::unique_ptr<Node>>& getChildren() const noexcept { return m_children; }

    size_t getChildCount() const { return m_children.size(); }

    void setParent(Node* parent) { m_parent = parent; }

    template<typename T, typename... Args>
    T* addComponent(Args&&... args)
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();
        m_components.push_back(std::move(component));
        return componentPtr;
    }

    void removeComponent(Component* component);

    const std::vector<std::unique_ptr<Component>>& getComponents() const noexcept { return m_components; }

    void update(float deltaTime);

    void updateTransform(const math::mat4& parentWorldMatrix, bool isParentDirty = false);

    void setPosition(const math::vec3& position) { m_transform.setPosition(position); }

    const math::vec3& getPosition() const noexcept { return m_transform.getPosition(); }

    void setRotation(const math::vec3& rotation) { m_transform.setRotation(rotation); }

    const math::vec3& getRotation() const noexcept { return m_transform.getRotation(); }

    void setScale(const math::vec3& scale) { m_transform.setScale(scale); }

    const math::vec3& getScale() const noexcept { return m_transform.getScale(); }

    math::Transform& getTransform() { return m_transform; }
    const math::Transform& getTransform() const noexcept { return m_transform; }

protected:
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Node>> m_children;
    std::vector<std::unique_ptr<Component>> m_components;
    math::Transform m_transform;
    bool m_visible = true;
};

} // namespace ocf
