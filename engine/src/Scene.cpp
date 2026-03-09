#include "ocf/Scene.h"
#include "ocf/Node.h"

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

} // namespace ocf
