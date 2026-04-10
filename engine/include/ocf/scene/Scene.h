// SPDX-License-Identifier: MIT

#pragma once

namespace ocf {

class Node;

class Scene {
public:
    Scene();
    virtual ~Scene();

    void update(float deltaTime);

    Node* getRoot() const { return m_root; }

protected:
    Node* m_root;
};

} // namespace ocf