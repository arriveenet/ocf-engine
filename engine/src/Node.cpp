#include "ocf/Component.h"
#include "ocf/Node.h"
#include "ocf/Renderer.h"
#include <algorithm>

namespace ocf {

Node::Node()
{}

Node::~Node()
{
    for (auto &child : m_children) {
        delete child;
    }
}

void Node::update(float deltaTime)
{
    // Update components
    for (auto component : m_components) {
        component->update(deltaTime);
    }

    // Update child nodes
    for (auto child : m_children) {
        child->update(deltaTime);
    }
}

void Node::render(Renderer* renderer)
{
    // Render child nodes
    for (auto child : m_children) {
        child->render(renderer);
    }
}

void Node::addChild(Node *node)
{
    node->m_parent = this;
    m_children.push_back(node);
}

void Node::removeChild(Node *node)
{
    auto iter = std::find(m_children.begin(), m_children.end(), node);
    if (iter != m_children.end()) {
        m_children.erase(iter);
        delete node;
    }
}

void Node::addComponent(Component *component)
{
    m_components.push_back(component);
}

void Node::removeComponent(Component *component)
{
    auto iter = std::find(m_components.begin(), m_components.end(), component);
    if (iter != m_components.end()) {
        m_components.erase(iter);
        delete component;
    }
}

} // namespace ocf