#include "ocf/RenderQueue.h"
#include "ocf/Program.h"
#include "ocf/VertexArrayObject.h"
#include <glad/gl.h>

namespace ocf {

void RenderQueue::push(const RenderCommand& command)
{
    m_commands.push_back(command);
}

void RenderQueue::flush()
{
    for (const auto& command : m_commands) {
        if (command.program == nullptr || command.vertexArrayObject == nullptr) {
            continue;
        }

        glUseProgram(command.program->getHandle());

        command.vertexArrayObject->bind();

        const GLenum mode = command.vertexArrayObject->getPrimitiveType();
        const size_t count = command.vertexArrayObject->getVertexCount();
        const size_t offset = command.vertexArrayObject->offset();

        if (command.vertexArrayObject->hasIndexBuffer()) {
            const GLenum indexType = command.vertexArrayObject->getIndexType();
            glDrawElements(mode, static_cast<GLint>(count), indexType,
                           reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)));
        }
        else {
            glDrawArrays(mode, static_cast<GLint>(offset), static_cast<GLsizei>(count));
        }
    }

    clear();
}

void RenderQueue::clear()
{
    m_commands.clear();
}

} // namespace ocf
