// SPDX-License-Identifier: MIT

#include "ocf/rhi/DeviceFactory.h"

#if OCF_RHI_D3D12
#include "ocf/rhi/d3d12/D3D12DeviceFactory.h"
#else
#include "ocf/rhi/vulkan/VulkanDeviceFactory.h"
#endif

namespace ocf {
namespace rhi {

DeviceFactory& DeviceFactory::getInstance()
{
    static std::unique_ptr<DeviceFactory> instance = CreateNativeFactory();
    return *instance;
}

std::unique_ptr<DeviceFactory> DeviceFactory::CreateNativeFactory()
{
    // The backend is chosen when the engine is built (OCF_RHI_BACKEND)
#if OCF_RHI_D3D12
    return std::make_unique<D3D12DeviceFactory>();
#else
    return std::make_unique<VulkanDeviceFactory>();
#endif
}

} // namespace rhi
} // namespace ocf
