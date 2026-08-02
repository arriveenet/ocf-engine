// SPDX-License-Identifier: MIT

#include "ocf/scene/Node.h"
#include "ocf/scene/Scene.h"

namespace ocf {

Scene::Scene()
{
    m_root = new Node();
}

Scene::~Scene()
{
    delete m_root;
    m_root = nullptr;
}

void Scene::update(float deltaTime)
{
    m_root->update(deltaTime);

    m_root->updateTransform(math::mat4(1.0f), true);
}

void Scene::traverseNodes(Node* node, const std::function<void(Node*)>& callback)
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
