#pragma once
#include "ocf/Transform.h"
#include <glm/glm.hpp>
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

    void setPosition(const glm::vec3& position);
    glm::vec3 getPosition() const;

    void setRotation(const glm::vec3& eulerAngles);
    glm::vec3 getRotation() const;

    void setScale(const glm::vec3& scale);
    glm::vec3 getScale() const;

    const Transform& getTransform() const { return m_transform; }
    Transform& getTransform() { return m_transform; }

    void addChild(Node* node);

    void removeChild(Node* node);

    void addComponent(Component* component);

    void removeComponent(Component* component);

protected:
    Node* m_parent = nullptr;
    std::string m_name;
    std::vector<Node*> m_children;
    std::vector<Component*> m_components;
    Transform m_transform;
};

} // namespace ocf
