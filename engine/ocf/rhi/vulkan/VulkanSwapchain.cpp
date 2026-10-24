// SPDX-License-Identifier: MIT
#include "ocf/rhi/vulkan/VulkanContext.h"
#include "ocf/rhi/vulkan/VulkanSwapchain.h"

#include "ocf/rhi/vulkan/VulkanDevice.h"

#include "ocf/core/Logger.h"

#include <algorithm>

namespace ocf {
namespace rhi {

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device)
    : m_device(device)
{
}

VulkanSwapchain::~VulkanSwapchain()
{
}

VulkanResult VulkanSwapchain::create(uint32_t width, uint32_t height, bool vsync)
{
    VkPhysicalDevice physicalDevice = m_device.getContext().getPhysicalDevice();
    VkDevice device = m_device.getDevice();
    VkSurfaceKHR surface = m_device.getContext().getSurface();

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width = width;
        extent.height = height;
    }

    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, formats.data());

    // Select output format
    VkSurfaceFormatKHR format = formats[0];
    for (auto& surfaceFormat : formats) {
        if (surfaceFormat.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            continue;
        }

        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM ||
            surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM) {
            format = surfaceFormat;
            break;
        }
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> presentModes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count,
                                              presentModes.data());

    // Select present mode. FIFO waits for the vertical blank and is always supported.
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsync) {
        auto isSupported = [&presentModes](VkPresentModeKHR mode) {
            return std::find(presentModes.begin(), presentModes.end(), mode) !=
                   presentModes.end();
        };
        if (isSupported(VK_PRESENT_MODE_MAILBOX_KHR)) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (isSupported(VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    auto imageCount = std::max(3u, caps.minImageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount + 1;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_swapchain;

    // Swapchain is generated after the GPU becomes idle
    vkDeviceWaitIdle(device);

    // Create swapchain
    VkSwapchainKHR swapchain;
    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
    if (result != VK_SUCCESS) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to create swapchain"));
    }
    m_swapchain = swapchain;
    m_imageFormat = format;
    m_imageExtent = extent;

    // Create image views for each swapchain image
    vkGetSwapchainImagesKHR(device, m_swapchain, &count, nullptr);
    m_images.resize(count);
    vkGetSwapchainImagesKHR(device, m_swapchain, &count, m_images.data());

    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo imageViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VkImageView view;
        vkCreateImageView(device, &imageViewCI, nullptr, &view);
        m_imageViews.push_back(view);
    }

    createFrameContext();

    OCF_LOG_INFO("Swapchain: {}x{}, {} images, present mode {}", extent.width, extent.height,
                 count, VulkanUtility::getPresentModeString(presentMode));

    return VulkanResult::Ok();
}

void VulkanSwapchain::destroy()
{
    destroyFrameContext();

    VkDevice device = m_device.getDevice();
    for (auto& view : m_imageViews) {
        vkDestroyImageView(device, view, nullptr);
    }
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
    m_images.clear();
    m_imageViews.clear();
}

VulkanResult VulkanSwapchain::acquireNextImage()
{
    VkDevice device = m_device.getDevice();

    assert(!m_presentSemaphoreList.empty());
    VkSemaphore acquireSemaphore = m_presentSemaphoreList.back();
    m_presentSemaphoreList.pop_back();

    VkResult result = vkAcquireNextImageKHR(device, m_swapchain, UINT64_MAX, acquireSemaphore,
                                            VK_NULL_HANDLE, &m_currentIndex);
    // VK_SUBOPTIMAL_KHR still acquires the image and signals the semaphore
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        // The semaphore was not used, so it can be reused for the next acquire
        m_presentSemaphoreList.push_back(acquireSemaphore);
        return VulkanResult::Err(VK_ERROR(result, "Failed to acquire next image"));
    }

    VkSemaphore oldSemaphore = m_frames[m_currentIndex].presentComplete;
    if (oldSemaphore != VK_NULL_HANDLE) {
        m_presentSemaphoreList.push_back(oldSemaphore);
    }
    m_frames[m_currentIndex].presentComplete = acquireSemaphore;
    return VulkanResult::Ok();
}

VulkanResult VulkanSwapchain::queuePresent(VkQueue queuePresent)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_frames[m_currentIndex].renderComplete;

    VkResult result = vkQueuePresentKHR(queuePresent, &presentInfo);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return VulkanResult::Err(VK_ERROR(result, "Failed to queuePresent"));
    }

    return VulkanResult::Ok();
}

VkSemaphore VulkanSwapchain::getPresentCompleteSemaphore() const
{
    return m_frames[m_currentIndex].presentComplete;
}

VkSemaphore VulkanSwapchain::getRenderCompleteSemaphore() const
{
    return m_frames[m_currentIndex].renderComplete;
}

void VulkanSwapchain::createFrameContext()
{
    VkDevice device = m_device.getDevice();
    m_frames.resize(m_images.size());
    for (auto& frame : m_frames) {
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        vkCreateSemaphore(device, &semaphoreCI, nullptr, &frame.renderComplete);
    }

    size_t presentCompleteSemaphoreCount = m_images.size() + 1;
    m_presentSemaphoreList.resize(presentCompleteSemaphoreCount);
    for (auto& semaphore : m_presentSemaphoreList) {
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore);
    }
}

void VulkanSwapchain::destroyFrameContext()
{
    VkDevice device = m_device.getDevice();
    for (auto& frame : m_frames) {
        vkDestroySemaphore(device, frame.renderComplete, nullptr);
        vkDestroySemaphore(device, frame.presentComplete, nullptr);
    }
    m_frames.clear();

    for (auto& semaphore : m_presentSemaphoreList) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    m_presentSemaphoreList.clear();
}


} // namespace rhi
} // namespace ocf
