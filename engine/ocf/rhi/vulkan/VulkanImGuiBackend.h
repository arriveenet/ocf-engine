// SPDX-License-Identifier: MIT
#pragma once

// Only part of the engine when it is built with Dear ImGui
#if OCF_ENABLE_IMGUI

#include "ocf/rhi/ImGuiBackend.h"

namespace ocf::rhi {

class VulkanDevice;

/** Draws ImGui with the Dear ImGui Vulkan backend, in a dynamic rendering pass of the swapchain */
class VulkanImGuiBackend : public ImGuiBackend {
public:
    explicit VulkanImGuiBackend(VulkanDevice& device);
    ~VulkanImGuiBackend() override;

    VulkanImGuiBackend(const VulkanImGuiBackend&) = delete;
    VulkanImGuiBackend& operator=(const VulkanImGuiBackend&) = delete;

    void newFrame() override;

    void renderDrawData(CommandBuffer& commandBuffer) override;

private:
    VulkanDevice& m_device;
};

} // namespace ocf::rhi

#endif // OCF_ENABLE_IMGUI
