// SPDX-License-Identifier: MIT
#include "VulkanBuffer.h"

#include "VulkanDevice.h"

namespace ocf::rhi {

template <typename T>
void BufferResource<T>::cleanup()
{
    if (m_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }
    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_memory, nullptr);
        m_memory = VK_NULL_HANDLE;
    }
    m_size = 0;
}

template <typename T>
void* BufferResource<T>::map()
{
    return nullptr;
}

template <typename T>
void BufferResource<T>::unmap()
{
}

template <typename T>
VkDescriptorBufferInfo BufferResource<T>::getDescriptorInfo() const
{
    return VkDescriptorBufferInfo{
        .buffer = m_buffer,
        .offset = 0,
        .range = m_size
    };
}

template <typename T>
bool BufferResource<T>::createBuffer(const VkBufferCreateInfo& createInfo,
                                     VkMemoryPropertyFlags memProps)
{
    auto result = vkCreateBuffer(m_device, &createInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = VulkanDevice::findMemoryType(memRequirements, memProps),
    };

    result = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
    m_size = createInfo.size;
    m_memProps = memProps;

    return true;
}

VulkanVertexBuffer::VulkanVertexBuffer(VkDevice device, Handle<RHIVertexBufferInfo> vbih)
    : BufferResource(device)
    , vbih(vbih)
{
}

void* VulkanVertexBuffer::map()
{
    if (!(m_memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        return nullptr;
    }

    void* mapped = nullptr;
    vkMapMemory(m_device, m_memory, 0, m_size, 0, &mapped);
    return mapped;
}

void VulkanVertexBuffer::unmap()
{
    if (!(m_memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        return;
    }
    vkUnmapMemory(m_device, m_memory);
}

bool VulkanVertexBuffer::initialize(VkDeviceSize size, VkMemoryPropertyFlags memProps)
{
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    setAccessFlags(VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
    return createBuffer(bufferInfo, memProps);
}

VulkanIndexBuffer::VulkanIndexBuffer(VkDevice device)
    : BufferResource(device)
{
}

void* VulkanIndexBuffer::map()
{
    if (!(m_memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        return nullptr;
    }

    void* mapped = nullptr;
    vkMapMemory(m_device, m_memory, 0, m_size, 0, &mapped);
    return mapped;
}

void VulkanIndexBuffer::unmap()
{
    if (!(m_memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        return;
    }
    vkUnmapMemory(m_device, m_memory);
}

bool VulkanIndexBuffer::initialize(VkDeviceSize size, VkMemoryPropertyFlags memProps)
{
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    setAccessFlags(VK_ACCESS_INDEX_READ_BIT);
    return createBuffer(bufferInfo, memProps);
}

StagingBuffer::StagingBuffer()
{
}

void* StagingBuffer::map()
{
    void* mapped = nullptr;
    vkMapMemory(m_device, m_memory, 0, m_size, 0, &mapped);
    return mapped;
}

void StagingBuffer::unmap()
{
    vkUnmapMemory(m_device, m_memory);
}

bool StagingBuffer::initialize(VkDevice device, VkDeviceSize size)
{
    m_device = device;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkMemoryPropertyFlags memProps =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    setAccessFlags(VK_ACCESS_HOST_WRITE_BIT);
    return createBuffer(bufferInfo, memProps);
}


void* UniformBuffer::map()
{
    void* mapped = nullptr;
    vkMapMemory(m_device, m_memory, 0, m_size, 0, &mapped);
    return mapped;
}

void UniformBuffer::unmap()
{
    vkUnmapMemory(m_device, m_memory);
}

bool UniformBuffer::initialize(VkDevice device, VkDeviceSize size)
{
    m_device = device;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkMemoryPropertyFlags memProps =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    setAccessFlags(VK_ACCESS_SHADER_READ_BIT);
    return createBuffer(bufferInfo, memProps);
}

} // namespace ocf::rhi
