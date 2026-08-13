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

    void setUpdateCallback(const std::function<void(float deltaTime)>& callback)
    {
        m_updateCallback = callback;
    }

protected:
    Node* m_root;
    std::function<void(float deltaTime)> m_updateCallback;
};

} // namespace ocf