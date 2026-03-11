#include "ocf/Renderer.h"
#include "ocf/RenderCommand.h"
#include "ocf/Scene.h"
#include <glad/gl.h>

namespace ocf {

Renderer::Renderer() {}

Renderer::~Renderer() {}

void Renderer::pushCommand(const RenderCommand& command)
{
    m_renderQueue.push(command);
}

void Renderer::render(Scene* scene)
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (scene) {
        scene->render(this);
    }

    m_renderQueue.flush();
}

} // namespace ocf
