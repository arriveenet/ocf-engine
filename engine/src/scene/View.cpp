// SPDX-License-Identifier: MIT
#include "ocf/scene/View.h"

#include <algorithm>

namespace ocf {

View::View() = default;

View::~View() = default;

void View::setScene(SceneGraph* sceneGraph)
{
    m_sceneGraphs.clear();
    addSceneGraph(sceneGraph);
}

void View::addSceneGraph(SceneGraph* sceneGraph)
{
    if (sceneGraph == nullptr) {
        return;
    }

    if (std::find(m_sceneGraphs.begin(), m_sceneGraphs.end(), sceneGraph) == m_sceneGraphs.end()) {
        m_sceneGraphs.push_back(sceneGraph);
    }
}

void View::removeSceneGraph(SceneGraph* sceneGraph)
{
    m_sceneGraphs.erase(std::remove(m_sceneGraphs.begin(), m_sceneGraphs.end(), sceneGraph),
                        m_sceneGraphs.end());
}

void View::setCamera(Camera* camera)
{
    m_camera = camera;
}

} // namespace ocf
