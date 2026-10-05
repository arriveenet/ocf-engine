// SPDX-License-Identifier: MIT
#include "ocf/renderer/RenderQueue.h"

#include <algorithm>

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
    // Draw opaque, then masked, then blended objects.
    // Blended objects are drawn back to front so that the objects behind them show through.
    std::stable_sort(m_renderCommands.begin(), m_renderCommands.end(),
                     [](const RenderCommand& a, const RenderCommand& b) {
                         if (a.alphaMode != b.alphaMode) {
                             return a.alphaMode < b.alphaMode;
                         }
                         if (a.alphaMode == AlphaMode::Blend) {
                             return a.distanceToCamera > b.distanceToCamera;
                         }
                         return false;
                     });
}

} // namespace ocf
