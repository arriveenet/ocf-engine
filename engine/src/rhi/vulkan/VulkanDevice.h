// SPDX - License - Identifier : MIT

#pragma once

#include "ocf/rhi/Device.h"

namespace ocf {
namespace rhi {

class VulkanContext;

class VulkanDevice : public Device {
public:
    VulkanDevice(VulkanContext& context);
    ~VulkanDevice() override;

    TextureHandle createTexture() override;

private:
    VulkanContext& m_context;
};

} // namespace rhi
} // namespace ocf
