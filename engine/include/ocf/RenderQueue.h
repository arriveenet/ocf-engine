#pragma once
#include "ocf/RenderCommand.h"
#include <vector>

namespace ocf {

class RenderQueue {
public:
    RenderQueue() = default;
    ~RenderQueue() = default;

    void push(const RenderCommand& command);

    void flush();

    void clear();

private:
    std::vector<RenderCommand> m_commands;
};

} // namespace ocf
