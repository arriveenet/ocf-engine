// SPDX-License-Identifier: MIT
#pragma once

#include "GpuResourceBase.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace ocf::rhi {

class IImageResource {
public:
    virtual ~IImageResource() = default;

    virtual VkFormat getFormat() const = 0;
    virtual VkExtent2D getExtent() const = 0;
    virtual uint32_t getMipmapCount() const = 0;

    virtual VkImage getImage() const = 0;
    virtual VkImageView getImageView() const = 0;
    virtual void setAccessFlag(const VkAccessFlags flags) = 0;
    virtual VkAccessFlags getAccessFlags() const = 0;

    virtual void setLayout(VkImageLayout layout) = 0;
    virtual VkImageLayout getLayout() const = 0;

    virtual VkImageSubresourceRange getSubresourceRange() const = 0;
};

template<typename T>
class ImageResource : public GpuResourceBase<T>, public IImageResource {
public:
    ImageResource(const ImageResource&) = delete;
    ImageResource& operator=(const ImageResource&) = delete;

    explicit ImageResource(VkDevice device)
        : m_device(device) {}
    virtual ~ImageResource() = default;

    virtual void cleanup() = 0;
    VkFormat getFormat() const override { return m_format; }
    VkExtent2D getExtent() const override { return m_extent; }
    uint32_t getMipmapCount() const override { return m_mipLevels; }

    VkImage getImage() const override { return m_image; }
    VkImageView getImageView() const override { return m_imageView; }
    void setAccessFlag(const VkAccessFlags flags) override { m_accessFlags = flags; }
    VkAccessFlags getAccessFlags() const override { return m_accessFlags; }

    void setLayout(VkImageLayout layout) override { m_layout = layout; }
    VkImageLayout getLayout() const override { return m_layout; }

    VkImageSubresourceRange getSubresourceRange() const override { return m_subresourceRange; }

protected:
    ImageResource() = default;

    VkDevice m_device = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageSubresourceRange m_subresourceRange{};
    VkAccessFlags m_accessFlags = VK_ACCESS_NONE;
    VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{};
    uint32_t m_mipLevels = 0;
};

class DepthBuffer : public ImageResource<DepthBuffer> {
    friend class GpuResourceBase<DepthBuffer>;

public:
    static std::shared_ptr<DepthBuffer> create(VkDevice device, VkExtent2D extent, VkFormat depthFormat)
    {
        auto image = GpuResourceBase::create();
        if (!image->initalize(device, extent, depthFormat)) {
            return nullptr;
        }
        return image;
    }

    ~DepthBuffer() override { cleanup(); }
    void cleanup() override;

    bool initalize(VkDevice device, VkExtent2D extent, VkFormat depthFormat);

private:
};

class Texture2D : public ImageResource<Texture2D> {
  friend class GpuResourceBase<Texture2D>;

public:
    static std::shared_ptr<Texture2D> create(VkDevice device, VkExtent2D extent,
                                             VkFormat depthFormat, uint32_t mipLevels)
    {
        auto image = GpuResourceBase::create();
        if (!image->initalize(device, extent, depthFormat, mipLevels)) {
            return nullptr;
        }
        return image;
    }

    ~Texture2D() override { cleanup();}
    void cleanup() override;

    bool initalize(VkDevice device, VkExtent2D extent, VkFormat format, uint32_t mipLevels);

    VkDescriptorImageInfo getDescriptorInfo(VkSampler sampler) const;
};

} // namespace ocf::rhi
