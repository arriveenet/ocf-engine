// SPDX-License-Identifier: MIT

#include "ocf/scene/Component.h"
#include "ocf/scene/Node.h"

#include <algorithm>

namespace ocf {

Node::Node()
{
}

Node::~Node()
{
}

Node* Node::createChild()
{
    auto child = std::make_unique<Node>();
    child->m_parent = this;
    Node* childPtr = child.get();
    m_children.push_back(std::move(child));
    return childPtr;
}

void Node::destroyChild(Node* child)
{
    m_children.erase(std::remove_if(m_children.begin(), m_children.end(),
                        [child](const std::unique_ptr<Node>& c) { return c.get() == child; }),
                     m_children.end());
}

void Node::removeComponent(Component* component)
{
    m_components.erase(std::remove_if(m_components.begin(), m_components.end(),
                        [component](const std::unique_ptr<Component>& c) { return c.get() == component; }),
                       m_components.end());
}

void Node::update(float deltaTime)
{
    for (auto& component : m_components) {
        component->update(deltaTime);
    }

    for (auto& child : m_children) {
        child->update(deltaTime);
    }
}

} // namespace ocf
