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

void Scene::update(float /*deltaTime*/)
{
}

} // namespace ocf
