// SPDX-License-Identifier: MIT
#include "ocf/renderer/RenderQueue.h"

namespace ocf {

RenderQueue::RenderQueue()
{
}

RenderQueue::~RenderQueue()
{
}

void RenderQueue::clear()
{
    m_renderCommands.clear();
}

bool RenderQueue::empty() const
{
    return m_renderCommands.empty();
}

void RenderQueue::addCommand(const RenderCommand& command)
{
    m_renderCommands.push_back(command);
}

void RenderQueue::sort()
{
    // TODO: Implement sorting logic for render commands based on material, pipeline, etc.
}

} // namespace ocf
