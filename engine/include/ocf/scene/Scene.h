// SPDX-License-Identifier: MIT

#pragma once

#include <functional>

namespace ocf {

class Node;

class Scene {
public:
    Scene();
    virtual ~Scene();

    void update(float deltaTime);

    void traverseNodes(Node* node, const std::function<void(Node*)>& callback);

    Node* getRoot() const { return m_root; }

protected:
    Node* m_root;
};

} // namespace ocf