#pragma once
#include <memory>

namespace ocf {

class Node;

class Scene {
public:
    Scene();
    virtual ~Scene();

    void update(float deltaTime);

    Node* getRoot() const { return m_root.get(); }

protected:
    std::unique_ptr<Node> m_root;
};

} // namespace ocf
