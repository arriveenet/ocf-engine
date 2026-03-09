#pragma once
#include <string>
#include <vector>

namespace ocf {

class Component;

class Node {
public:
    Node();
    virtual ~Node();

    void update(float deltaTime);

    void setParent(Node* parent) { m_parent = parent; }

    void setName(const std::string &name) { m_name = name; }

    std::string getName() const { return m_name; }

    void addChild(Node* node);

    void removeChild(Node* node);

    void addComponent(Component* component);

    void removeComponent(Component* component);

protected:
    Node* m_parent = nullptr;
    std::string m_name;
    std::vector<Node*> m_children;
    std::vector<Component*> m_components;
};

} // namespace ocf
