// SPDX-License-Identifier: MIT
#pragma once

// Only part of the engine when it is built with Dear ImGui
#if OCF_ENABLE_IMGUI

#include "ocf/core/event/EventDispatcher.h"

namespace ocf {

class Window;

namespace rhi {
class CommandBuffer;
class Device;
class VulkanDevice;
} // namespace rhi

/**
 * @brief Dear ImGui debug UI, drawn over the views every frame.
 *
 * Owns the ImGui context and its GLFW platform and Vulkan renderer backends. Scenes build
 * their windows with the ImGui API in Scene::onImGui().
 *
 * ImGui reads the input from GLFW itself. While it uses the mouse or the keyboard (e.g. a
 * window is hovered or a text field is active), those input events are marked handled before
 * they reach the scenes.
 */
class ImGuiLayer {
public:
    /** @note Must be created before the scene manager, so that it receives the events first. */
    ImGuiLayer(Window& window, rhi::Device& device, EventDispatcher& eventDispatcher);
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    /** @brief Start an ImGui frame. The ImGui API can be used until render(). */
    void beginFrame();

    /** @brief End the ImGui frame and record its draw commands into the current pass. */
    void render(rhi::CommandBuffer& commandBuffer);

    /** @brief End the ImGui frame without drawing it, for a frame that is not rendered. */
    void endFrame();

private:
    rhi::VulkanDevice& m_device;
    Subscription m_keyboardSubscription;
    Subscription m_mouseSubscription;
};

} // namespace ocf

#endif // OCF_ENABLE_IMGUI
