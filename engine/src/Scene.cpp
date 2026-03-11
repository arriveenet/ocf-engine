#include "ocf/Scene.h"
#include "ocf/Camera.h"
#include "ocf/Node.h"
#include "ocf/Renderer.h"
#include <algorithm>

namespace ocf {

Scene::Scene()
{ 
    m_root = std::make_unique<Node>();
}

Scene::~Scene() {}

void Scene::update(float deltaTime)
{
    m_root->update(deltaTime);
}

void Scene::render(Renderer* renderer)
{
    // Pull-type rendering: for each active camera, traverse the node tree
    // and collect render commands. Each camera represents an independent
    // render pass (e.g. different viewport, view/projection matrix).
    for (auto* camera : m_cameras) {
        if (!camera->isActive()) {
            continue;
        }
        m_root->render(renderer);
    }
}

void Scene::addCamera(Camera* camera)
{
    m_cameras.push_back(camera);
    std::stable_sort(m_cameras.begin(), m_cameras.end(),
        [](const Camera* a, const Camera* b) {
            return a->getDepth() < b->getDepth();
        });
}

void Scene::removeCamera(Camera* camera)
{
    auto it = std::find(m_cameras.begin(), m_cameras.end(), camera);
    if (it != m_cameras.end()) {
        m_cameras.erase(it);
    }
}

} // namespace ocf
