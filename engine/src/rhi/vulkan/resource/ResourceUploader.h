// SPDX-License-Identifier: MIT
#pragma once

#include "vulkan/VulkanBuffer.h"
#include "vulkan/VulkanDevice.h"

#include <memory>
#include <vector>

namespace ocf::rhi {

struct TextureUploadRequest {
    std::shared_ptr<StagingBuffer> staging;
    std::vector<VkBufferImageCopy> copyRegions;

    VkAccessFlags nextAccessFlags;
    VkImageLayout nextLayout;
    VkPipelineStageFlags nextStageFlags;
};

class ResourceUploader {
public:
    ResourceUploader() = default;
    ~ResourceUploader() = default;

    bool initialize(VulkanDevice& device);
    void cleanup();
    
    bool uploadBuffer(IBufferResource* target, const void* pData, size_t size,
                      VkAccessFlags nextAccessMask);

    bool uploadImage(std::shared_ptr<IImageResource> target, TextureUploadRequest& request);

    void submitAndWait();

private:
    struct PendingTransfer {
        std::shared_ptr<StagingBuffer> stagingBuffer;
        IBufferResource* destinationBuffer;
        VkAccessFlags dstAccessMask;
    };

    struct PendingImageTransfer {
        std::shared_ptr<StagingBuffer> stagingBuffer;
        std::shared_ptr<IImageResource> destinationTexture;
        std::vector<VkBufferImageCopy> copyRegions;
        bool genMipmaps = false;
        VkAccessFlags dstAccessMask;
        VkImageLayout dstImageLayout;
        VkPipelineStageFlags dstStageFlags;
    };

    void createMipmap(std::shared_ptr<CommandBuffer> commandBuffer, PendingImageTransfer& entry);

    VulkanDevice* m_vulkanDevice = nullptr;
    VkFence m_transferFence = VK_NULL_HANDLE;
    std::vector<PendingTransfer> m_transferEntries;
    std::vector<PendingImageTransfer> m_transferImageEntries;
};


} // namespace ocf::rhi
