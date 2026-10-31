// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Device.h"
#include "ocf/rhi/vulkan/VulkanUtility.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace ocf {
namespace rhi {

class VulkanDevice;

class VulkanSwapchain : public RHISwapchain {
public:
    explicit VulkanSwapchain(VulkanDevice& device);
    ~VulkanSwapchain();

    /** @param vsync FIFO if true, otherwise MAILBOX or IMMEDIATE when the surface supports it */
    VulkanResult create(uint32_t width, uint32_t height, bool vsync);

    /**
     * @brief Wait for the GPU, then create the swapchain again at the current surface size.
     *
     * Fails without touching the swapchain when the surface has no size (minimized window).
     * width and height are only used when the surface leaves the size to the swapchain.
     */
    VulkanResult recreate(uint32_t width, uint32_t height);
    void destroy();

    /** @brief Whether the surface size no longer matches the swapchain, as after a resize. */
    bool hasResized(uint32_t width, uint32_t height) const;

    /** @brief Whether acquire or present reported the swapchain as suboptimal or out of date. */
    bool isOutOfDate() const noexcept { return m_outOfDate; }

    VulkanResult acquireNextImage();
    VulkanResult queuePresent(VkQueue queuePresent);

    operator VkSwapchainKHR() const noexcept { return m_swapchain; }
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

    /** The current surface size, or the given size when the surface leaves it undefined */
    VkExtent2D querySurfaceExtent(uint32_t width, uint32_t height) const;

    VulkanDevice& m_device;
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
    bool m_vsync = true;
    bool m_outOfDate = false;
};

} // namespace rhi
} // namespace ocf
