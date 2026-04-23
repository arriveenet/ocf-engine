// SPDX-License-Identifier: MIT

#pragma once

#include "rhi/HandleAllocator.h"

#include "ocf/rhi/Device.h"

#include <vulkan/vulkan.h>

namespace ocf::rhi {

class VulkanContext;

struct VulkanShader : public RHIShader {
    struct VK {
        VkShaderModule id = VK_NULL_HANDLE;
    } vk;

};

class VulkanDevice : public Device {
public:
    VulkanDevice(VulkanContext& context);
    ~VulkanDevice() override;

    TextureHandle createTexture() override;

    ShaderHandle createShader(std::string_view filename) override;

    SwapchainHandle createSwapchain(Window* window, uint32_t width, uint32_t height) override;

private:
    template <typename D, typename... ARGS>
    Handle<D> initHandle(ARGS&&... args)
    {
        return m_handleAllocator.allocateAndConstruct<D>(std::forward<ARGS>(args)...);
    }

    template <typename D, typename B, typename... ARGS>
    std::enable_if_t<std::is_base_of_v<B, D>, D>* construct(Handle<B> const& handle, ARGS&&... args)
    {
        return m_handleAllocator.destroyAndConstruct<D, B>(handle, std::forward<ARGS>(args)...);
    }

    template <typename B, typename D, typename = std::enable_if_t<std::is_base_of_v<B, D>, D>>
    void destruct(Handle<B>& handle, D const* p) noexcept
    {
        m_handleAllocator.deallocate(handle, p);
    }

    template <typename Dp, typename B>
    std::enable_if_t<std::is_pointer_v<Dp> && std::is_base_of_v<B, std::remove_pointer_t<Dp>>, Dp>
    handle_cast(Handle<B>& handle)
    {
        return m_handleAllocator.handle_cast<Dp, B>(handle);
    }

    template <typename Dp, typename B>
    std::enable_if_t<std::is_pointer_v<Dp> && std::is_base_of_v<B, std::remove_pointer_t<Dp>>, Dp>
    handle_cast(const Handle<B>& handle)
    {
        return m_handleAllocator.handle_cast<Dp, B>(handle);
    }

private:
    VulkanContext& m_context;
    HandleAllocatorVK m_handleAllocator;
};

} // namespace ocf::rhi
