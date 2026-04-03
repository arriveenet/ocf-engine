#include "VulkanSwapchain.h"

#include "VulkanContext.h"

namespace ocf {
namespace rhi {

VulkanSwapchain::VulkanSwapchain(VulkanContext& context)
    : m_context(context)
{
}

VulkanResult VulkanSwapchain::create(uint32_t width, uint32_t height)
{
    VkPhysicalDevice physicalDevice = m_context.getPhysicalDevice();
    VkDevice device = m_context.getDevice();
    VkSurfaceKHR surface = m_context.getSurface();

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
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
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

    return VulkanResult::Ok();
}

void VulkanSwapchain::destroy()
{
}

VulkanResult VulkanSwapchain::acquireNextImage()
{
    return VulkanResult::Ok();
}

VulkanResult VulkanSwapchain::queuePresent(VkQueue presentQueue)
{
    return VulkanResult::Ok();
}

VkSemaphore VulkanSwapchain::getPresentCompleteSemaphore() const
{
    return VkSemaphore();
}

VkSemaphore VulkanSwapchain::getRenderCompleteSemaphore() const
{
    return VkSemaphore();
}

void VulkanSwapchain::createFrameContext()
{
}

void VulkanSwapchain::destroyFrameContext()
{
}


} // namespace rhi
} // namespace ocf
