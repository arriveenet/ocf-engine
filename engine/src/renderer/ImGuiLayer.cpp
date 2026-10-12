// SPDX-License-Identifier: MIT
#include "ImGuiLayer.h"

#if OCF_ENABLE_IMGUI

#include "platform/GLFWWindow.h"
#include "rhi/vulkan/VulkanCommandBuffer.h"
#include "rhi/vulkan/VulkanContext.h"
#include "rhi/vulkan/VulkanDevice.h"
#include "rhi/vulkan/VulkanSwapchain.h"
#include "rhi/vulkan/VulkanUtility.h"

#include "ocf/core/event/KeyboardEvent.h"
#include "ocf/core/event/MouseEvent.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace ocf {

using namespace rhi;

namespace {

void checkVkResult(VkResult result)
{
    if (result != VK_SUCCESS) {
        VulkanUtility::logError(VK_ERROR(result, "Dear ImGui Vulkan backend call failed"));
    }
}

} // namespace

// The engine only creates GLFW windows and Vulkan devices, which are what the backends use
ImGuiLayer::ImGuiLayer(Window& window, Device& device, EventDispatcher& eventDispatcher)
    : m_device(static_cast<VulkanDevice&>(device))
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // ImGui chains the callbacks already set on the window, so the engine still gets the input
    ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWWindow&>(window).getHandle(), true);

    VulkanContext& context = m_device.getContext();
    const VulkanSwapchain* swapchain = m_device.getSwapchain();
    const VkFormat colorFormat = swapchain->getImageFormat().format;

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_3; // The version VulkanContext creates the instance with
    initInfo.Instance = context.getInstance();
    initInfo.PhysicalDevice = context.getPhysicalDevice();
    initInfo.Device = m_device.getDevice();
    initInfo.QueueFamily =
        VulkanUtility::findQueueFamilies(context.getPhysicalDevice()).graphicsFamily.value();
    initInfo.Queue = m_device.getGraphicsQueue();
    initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
    // ImGui reuses its vertex buffers every ImageCount frames, so it must cover the frames in flight
    initInfo.MinImageCount = VulkanDevice::MAX_FRAMES_IN_FLIGHT;
    initInfo.ImageCount = swapchain->getImageCount();
    // Drawn in a pass of the renderer whose only attachment is the swapchain image
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
    };
    initInfo.CheckVkResultFn = checkVkResult;
    ImGui_ImplVulkan_Init(&initInfo);

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
    // The frames in flight may still use the ImGui buffers and textures
    vkDeviceWaitIdle(m_device.getDevice());

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::render(CommandBuffer& commandBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    static_cast<VulkanCommandBuffer&>(commandBuffer).getHandle());
}

} // namespace ocf

#endif // OCF_ENABLE_IMGUI
