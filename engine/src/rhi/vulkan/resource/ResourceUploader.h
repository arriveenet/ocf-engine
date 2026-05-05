// SPDX-License-Identifier: MIT
#pragma once

#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanDevice.h"

#include <vector>

namespace ocf::rhi {

class ResourceUploader {
public:
    ResourceUploader() = default;
    ~ResourceUploader() = default;

    bool initialize(VulkanDevice& device);
    void cleanup();
    
    bool uploadBuffer(IBufferResource* target, const void* pData, size_t size,
                      VkAccessFlags nextAccessMask);

    void submitAndWait();

private:
    struct PendingTransfer {
        std::shared_ptr<StagingBuffer> stagingBuffer;
        IBufferResource* destinationBuffer;
        VkAccessFlags dstAccessMask;
    };
    VulkanDevice* m_vulkanDevice = nullptr;
    VkFence m_transferFence = VK_NULL_HANDLE;
    std::vector<PendingTransfer> m_transferEntries;
};


} // namespace ocf::rhi
