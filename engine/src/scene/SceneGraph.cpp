// SPDX-License-Identifier: MIT

#include "ocf/scene/Node.h"
#include "ocf/scene/SceneGraph.h"

namespace ocf {

SceneGraph::SceneGraph()
{
    m_root = new Node();
}

SceneGraph::~SceneGraph()
{
    delete m_root;
    m_root = nullptr;
}

void SceneGraph::update(float deltaTime)
{
    if (m_updateCallback) {
        m_updateCallback(deltaTime);
    }

    m_root->update(deltaTime);

    m_root->updateTransform(math::mat4(1.0f), true);
}

void SceneGraph::traverseNodes(Node* node, const std::function<void(Node*)>& callback)
{
    if (node == nullptr) {
        return;
    }

    callback(node);

    for (const auto& child : node->getChildren()) {
        traverseNodes(child.get(), callback);
    }
}

} // namespace ocf
