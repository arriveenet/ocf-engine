// SPDX-License-Identifier: MIT

#include "VulkanDevice.h"

#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"

#include "ocf/core/Logger.h"

#include <fstream>

namespace ocf {
namespace rhi {

VulkanDevice::VulkanDevice(VulkanContext& context)
    : m_context(context)
    , m_handleAllocator("Handles", 0x400000u)
{
}

VulkanDevice::~VulkanDevice()
{
    m_context.terminate();
}

TextureHandle VulkanDevice::createTexture()
{
    return TextureHandle();
}

ShaderModuleHandle VulkanDevice::createShaderModule(std::string_view filename)
{
    Handle<VulkanShaderModule> handle = initHandle<VulkanShaderModule>();

    VulkanShaderModule* shader = construct<VulkanShaderModule>(handle);

    std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        OCF_LOG_ERROR("Failded to open shader file: {}", filename.data());
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0).read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkDevice device = m_context.getDevice();
    auto result = vkCreateShaderModule(device, &createInfo, nullptr, &shader->vk.id);
    if (result != VK_SUCCESS) {
        OCF_LOG_ERROR("Failded to create shader module from: {}", filename.data());
    }
 
    return Handle<RHIShaderModule>{handle.getId()};
}

SwapchainHandle VulkanDevice::createSwapchain(Window* window, uint32_t width, uint32_t height)
{
    Handle<VulkanSwapchain> handle = m_handleAllocator.allocate<Handle<VulkanSwapchain>>();
    m_context.createSwapchain(window, width, height);

    // TODO: Handle create result
    return Handle<RHISwapchain>{handle.getId()};
}

std::shared_ptr<CommandBuffer> VulkanDevice::createCommandBuffer()
{
    return m_context.createCommandBuffer();
}

} // namespace rhi
} // namespace ocf
