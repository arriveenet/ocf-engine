// SPDX-License-Identifier: MIT
#include "ocf/scene/View.h"

namespace ocf {

View::View() = default;

View::~View() = default;

void View::setScene(Scene* scene)
{
    m_scene = scene;
}

void View::setCamera(Camera* camera)
{
    m_camera = camera;
}

} // namespace ocf
