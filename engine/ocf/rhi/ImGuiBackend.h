// SPDX-License-Identifier: MIT
#pragma once

// Only part of the engine when it is built with Dear ImGui
#if OCF_ENABLE_IMGUI

#include "ocf/core/compiler.h"

namespace ocf::rhi {

class CommandBuffer;

/**
 * @brief The Dear ImGui renderer backend of a device, created by Device::createImGuiBackend().
 *
 * It is initialized with the ImGui context current at its creation, and shut down when destroyed.
 * It must be destroyed before the ImGui context and the device.
 */
class OCF_API ImGuiBackend {
public:
    virtual ~ImGuiBackend() = default;

    /** @brief Prepare the backend for a new ImGui frame, before ImGui::NewFrame(). */
    virtual void newFrame() = 0;

    /** @brief Record the draw commands of ImGui::GetDrawData() into the current pass. */
    virtual void renderDrawData(CommandBuffer& commandBuffer) = 0;
};

} // namespace ocf::rhi

#endif // OCF_ENABLE_IMGUI
