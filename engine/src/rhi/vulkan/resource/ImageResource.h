// SPDX-License-Identifier: MIT
#pragma once

#include "GpuResourceBase.h"

#include <vulkan/vulkan.h>

namespace ocf::rhi {

class IImageResource {
public:
    virtual VkFormat getFormat() const = 0;
    virtual VkExtent2D getExtent() const = 0;
    virtual uint32_t getMipmapCount() const = 0;

    virtual VkImage getImage() const = 0;
    virtual void setAccessFlag(const VkAccessFlags flags) = 0;
    virtual VkAccessFlags getAccessFlags() const = 0;

    virtual void setLayout(VkImageLayout layout) = 0;
    virtual VkImageLayout getLayout() const = 0;
};

template<typename T>
class ImageResource : public GpuResourceBase<T>, public IImageResource {
public:
    ImageResource(const ImageResource&) = delete;
    ImageResource& operator=(const ImageResource&) = delete;

    explicit ImageResource(VkDevice device)
        : m_device(device) {};
    virtual ~ImageResource() = default;

    virtual void cleanup() = 0;
    VkFormat getFormat() const override { return m_format; }
    VkExtent2D getExtent() const override { return m_extent; }
    uint32_t getMipmapCount() const override { return m_mipLevels; }

    VkImage getImage() const { return m_image; }
    void setAccessFlag(const VkAccessFlags flags) override { m_accessFlags = flags; }
    VkAccessFlags getAccessFlags() const override { return m_accessFlags; }

    void setLayout(VkImageLayout layout) override { m_layout = layout; }
    VkImageLayout getLayout() const override { return m_layout; }

protected:
    ImageResource() = default;

    VkDevice m_device;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageSubresourceRange m_subresourceRange{};
    VkAccessFlags m_accessFlags = VK_ACCESS_NONE;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{};
    uint32_t m_mipLevels;
};

class DepthBuffer : public ImageResource<DepthBuffer> {
    friend class GpuResourceBase<DepthBuffer>;

public:
    DepthBuffer(VkDevice device);
    ~DepthBuffer() override { cleanup(); }
    void cleanup() override;

    bool initalize(VkExtent2D extent, VkFormat depthFormat);

    VkImageView getImageView() const { return m_imageView; }

private:
    VkImageView m_imageView{};
};

} // namespace ocf::rhi