// SPDX - License - Identifier : MIT

#pragma once

#include "ocf/core/Object.h"
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

    void update(float deltaTime);

protected:
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Node>> m_children;
    std::vector<std::unique_ptr<Component>> m_components;
};

} // namespace ocf
