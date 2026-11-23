// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanImGuiBackend.h"

#if OCF_ENABLE_IMGUI

#include "ocf/rhi/vulkan/VulkanCommandBuffer.h"
#include "ocf/rhi/vulkan/VulkanContext.h"
#include "ocf/rhi/vulkan/VulkanDevice.h"
#include "ocf/rhi/vulkan/VulkanSwapchain.h"
#include "ocf/rhi/vulkan/VulkanUtility.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

namespace ocf::rhi {

namespace {

void checkVkResult(VkResult result)
{
    if (result != VK_SUCCESS) {
        VulkanUtility::logError(VK_ERROR(result, "Dear ImGui Vulkan backend call failed"));
    }
}

} // namespace

VulkanImGuiBackend::VulkanImGuiBackend(VulkanDevice& device)
    : m_device(device)
{
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
}

VulkanImGuiBackend::~VulkanImGuiBackend()
{
    // The frames in flight may still use the ImGui buffers and textures
    vkDeviceWaitIdle(m_device.getDevice());

    ImGui_ImplVulkan_Shutdown();
}

void VulkanImGuiBackend::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
}

void VulkanImGuiBackend::renderDrawData(CommandBuffer& commandBuffer)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    static_cast<VulkanCommandBuffer&>(commandBuffer).getHandle());
}

} // namespace ocf::rhi

#endif // OCF_ENABLE_IMGUI
