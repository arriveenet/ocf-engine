#pragma once

namespace ocf {

class Node;

class Component {
public:
    Component();
    virtual ~Component();

    virtual void update(float deltaTime) = 0;

protected:
    Node *m_pOwner = nullptr;
};

} // namespace ocf
