// SPDX-License-Identifier: MIT
#include "ImageResource.h"

#include "vulkan/VulkanDevice.h"
#include <vulkan/vulkan_core.h>

namespace ocf::rhi {

void DepthBuffer::cleanup()
{
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
    }
    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
    }
    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_memory, nullptr);
    }
    m_image = VK_NULL_HANDLE;
    m_imageView = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
}

bool DepthBuffer::initalize(VkDevice device, VkExtent2D extent, VkFormat depthFormat)
{
    m_device = device;
    m_format = depthFormat;
    m_extent = extent;
    m_mipLevels = 1;

    VkImageCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = m_format,
        .extent = {extent.width, extent.height, 1}, 
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(m_device, &createInfo, nullptr, &m_image) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = VulkanDevice::findMemoryType(memRequirements, memProps),
    };
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        return false;
    }

    if (vkBindImageMemory(m_device, m_image, m_memory, 0) != VK_SUCCESS) {
        return false;
    }

    m_subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = createInfo.mipLevels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageViewCreateInfo viewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = createInfo.format,
        .subresourceRange = m_subresourceRange,
    };
    if (vkCreateImageView(m_device, &viewCreateInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void Texture2D::cleanup()
{
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
    }
    if (m_image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
    }
    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_memory, nullptr);
    }

    m_image = VK_NULL_HANDLE;
    m_imageView = VK_NULL_HANDLE;
    m_memory = VK_NULL_HANDLE;
}

bool Texture2D::initalize(VkDevice device, VkExtent2D extent, VkFormat format, uint32_t mipLevels)
{
    m_device = device;
    m_extent = extent;
    m_format = format;
    m_mipLevels = m_mipLevels;

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;
    usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkImageCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = m_format,
        .extent = {extent.width, extent.height, 1},
        .mipLevels = m_mipLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(device, &createInfo, nullptr , &m_image) != VK_SUCCESS) {
        return false;
    }

    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_image, &memRequirements);
    VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = VulkanDevice::findMemoryType(memRequirements, memProps),
    };
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        return false;
    }

    if (vkBindImageMemory(m_device, m_image, m_memory, 0) != VK_SUCCESS) {
        return false;
    }

    m_subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0, .levelCount = mipLevels,
        .baseArrayLayer = 0, .layerCount = 1,
    };

    // Create image view
    VkImageViewCreateInfo viewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = createInfo.format,
        .subresourceRange = m_subresourceRange,
    };
    if (vkCreateImageView(m_device, &viewCreateInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return false;
    }

    return true;
}

VkDescriptorImageInfo Texture2D::getDescriptorInfo(VkSampler sampler) const
{
    return VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = m_imageView,
        .imageLayout = m_layout,
    };
}

template class ImageResource<DepthBuffer>;
template class ImageResource<Texture2D>;

} // namespace ocf::rhi
