// SPDX-License-Identifier: MIT
#include "ocf/renderer/ImGuiLayer.h"

#if OCF_ENABLE_IMGUI

#include "ocf/platform/GLFWWindow.h"
#include "ocf/rhi/Device.h"
#include "ocf/rhi/ImGuiBackend.h"

#include "ocf/core/event/KeyboardEvent.h"
#include "ocf/core/event/MouseEvent.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>

namespace ocf {

using namespace rhi;

// The engine only creates GLFW windows, which is what the platform backend uses
ImGuiLayer::ImGuiLayer(Window& window, Device& device, EventDispatcher& eventDispatcher)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // ImGui chains the callbacks already set on the window, so the engine still gets the input.
    // The renderer backend comes from the device, so the client API is left unspecified.
    ImGui_ImplGlfw_InitForOther(static_cast<GLFWWindow&>(window).getHandle(), true);

    m_rendererBackend = device.createImGuiBackend();

    m_keyboardSubscription = eventDispatcher.subscribe<KeyboardEvent>([](KeyboardEvent& event) {
        if (ImGui::GetIO().WantCaptureKeyboard) {
            event.setHandled(true);
        }
    });
    m_mouseSubscription = eventDispatcher.subscribe<MouseEvent>([](MouseEvent& event) {
        if (ImGui::GetIO().WantCaptureMouse) {
            event.setHandled(true);
        }
    });
}

ImGuiLayer::~ImGuiLayer()
{
    m_rendererBackend.reset();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame()
{
    m_rendererBackend->newFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::render(CommandBuffer& commandBuffer)
{
    ImGui::Render();
    m_rendererBackend->renderDrawData(commandBuffer);
}

void ImGuiLayer::endFrame()
{
    ImGui::EndFrame();
}

} // namespace ocf

#endif // OCF_ENABLE_IMGUI
