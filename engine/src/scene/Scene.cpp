// SPDX-License-Identifier: MIT
#include "ocf/scene/Scene.h"

#include "ocf/scene/SceneGraph.h"
#include "ocf/scene/SceneManager.h"
#include "ocf/scene/View.h"

namespace ocf {

Scene::Scene()
{
}

Scene::~Scene()
{
    m_views.clear();
    m_sceneGraphs.clear();
}

void Scene::close()
{
    if (m_sceneManager != nullptr) {
        m_sceneManager->remove(this);
    }
}

SceneGraph* Scene::createSceneGraph()
{
    m_sceneGraphs.push_back(std::make_unique<SceneGraph>());
    return m_sceneGraphs.back().get();
}

View* Scene::createView()
{
    m_views.push_back(std::make_unique<View>());
    return m_views.back().get();
}

void Scene::addChildScene(int layer, std::unique_ptr<Scene> child)
{
    m_sceneManager->enqueue(std::move(child), layer, this, false);
}

} // namespace ocf
