#pragma once

#include "ocf/rhi/Device.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace ocf::rhi {

template<typename T>
class GpuResourceBase {
public:
    // Copy is prohibited
    GpuResourceBase(const GpuResourceBase&) = delete;
    GpuResourceBase& operator=(const GpuResourceBase&) = delete;

    virtual ~GpuResourceBase() = default;

    static std::shared_ptr<T> create() { return std::shared_ptr<T>(new T()); }

protected:
    GpuResourceBase() = default;
};

class IBufferResource {
public:
    virtual bool isHostAccessible() const = 0;

    virtual void setAccessFlags(const VkAccessFlags flags) = 0;
    virtual VkAccessFlags getAccessFlags() const = 0;

    virtual VkBuffer getBuffer() const = 0;
    virtual VkDeviceSize getBufferSize() const = 0;

    virtual void* map() = 0;
    virtual void unmap() = 0;

    virtual VkDescriptorBufferInfo getDescriptorInfo() const = 0;
};

template<typename T>
class BufferResource : public GpuResourceBase<T>, public IBufferResource {
public:
    BufferResource(const BufferResource&) = delete;
    BufferResource& operator=(const BufferResource&) = delete;

    explicit BufferResource(VkDevice device)
        : m_device{device} {};
    virtual ~BufferResource() { cleanup(); }
    virtual void cleanup();

    bool isHostAccessible() const override
    {
        return (m_memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    }

    void setAccessFlags(const VkAccessFlags flags) override { m_accessFlags = flags; }
    VkAccessFlags getAccessFlags() const { return m_accessFlags; }

    VkBuffer getBuffer() const override { return m_buffer; }
    VkDeviceSize getBufferSize() const override { return m_size; }

    void* map() override;
    void unmap() override;

    VkDescriptorBufferInfo getDescriptorInfo() const override;

protected:
    bool createBuffer(const VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags memProps);

    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_buffer{};
    VkDeviceMemory m_memory{};
    VkDeviceSize m_size{};
    VkMemoryPropertyFlags m_memProps{};
    VkAccessFlags m_accessFlags = VK_ACCESS_NONE;
};

struct VulkanVertexBuffer : public RHIVertexBuffer, BufferResource<VulkanVertexBuffer> {
private:
    friend class GpuResourceBase<VulkanVertexBuffer>;

    VulkanVertexBuffer() = default;

public:
    explicit VulkanVertexBuffer(VkDevice device);

public:
    ~VulkanVertexBuffer() override = default;

    void* map() override;
    void unmap() override;

    bool initialize(VkDeviceSize size, VkMemoryPropertyFlags memProps);

};

} // namespace ocf::rhi
