// SPDX-License-Identifier: MIT
#pragma once

#include "VulkanUtility.h"

#include "ocf/rhi/Device.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace ocf {
namespace rhi {

class VulkanContext;

class VulkanSwapchain : public RHISwapchain {
public:
    explicit VulkanSwapchain(VulkanContext& context);

    VulkanResult create(uint32_t width, uint32_t height);
    void destroy();

    VulkanResult acquireNextImage();
    VulkanResult queuePresent(VkQueue presentQueue);

    operator const VkSwapchainKHR() const noexcept { return m_swapchain; }
    VkSurfaceFormatKHR getImageFormat() const noexcept { return m_imageFormat; }
    VkExtent2D getExtent() const noexcept { return m_imageExtent; }

    uint32_t getCurrentIndex() const noexcept { return m_currentIndex; }
    uint32_t getImageCount() const noexcept { return uint32_t(m_images.size()); }
    VkImage getCurrentImage() const noexcept { return m_images[m_currentIndex]; }
    VkImageView getCurrentImageView() const noexcept { return m_imageViews[m_currentIndex]; }

    VkSemaphore getPresentCompleteSemaphore() const;
    VkSemaphore getRenderCompleteSemaphore() const;
    std::vector<VkImageView> getImageViews() const noexcept { return m_imageViews; }

private:
    void createFrameContext();
    void destroyFrameContext();

    VulkanContext& m_context;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    uint32_t m_currentIndex = 0;

    VkSurfaceFormatKHR m_imageFormat{};
    VkExtent2D m_imageExtent{};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    struct FrameContext {
        VkSemaphore renderComplete = VK_NULL_HANDLE;
        VkSemaphore presentComplete = VK_NULL_HANDLE;
    };
    std::vector<FrameContext> m_frames;
    std::vector<VkSemaphore> m_presentSemaphoreList;

};

} // namespace rhi
} // namespace ocf
