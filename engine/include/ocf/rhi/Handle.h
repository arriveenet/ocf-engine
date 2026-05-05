// SPDX-License-Identifier: MIT

#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

namespace ocf {
namespace rhi {

struct RHITexture;
struct RHIVertexBuffer;
struct RHIVertexBufferInfo;
struct RHIIndexBuffer;
struct RHIShaderModule;
struct RHIDescriptorSetLayout;
struct RHIDescriptorSet;
struct RHIPipeline;
struct RHISwapchain;

class HandleBase {
public:
    using HandleId = uint32_t;
    static constexpr HandleId nullid = UINT32_MAX;

    HandleBase() noexcept
        : object(nullid)
    {
    }

    explicit HandleBase(HandleId id) noexcept
        : object(id)
    {
        assert(object != nullid);
    }

    explicit operator bool() const noexcept { return object != nullid; }

    void clear() noexcept { object = nullid; }

    HandleId getId() const { return object; }

protected:
    HandleBase(const HandleBase&) = default;
    HandleBase& operator=(const HandleBase&) = default;

    HandleBase(HandleBase&& rhs) noexcept
        : object(rhs.object)
    {
        rhs.object = nullid;
    }

    HandleBase& operator=(HandleBase&& rhs) noexcept
    {
        if (this != &rhs) {
            object = rhs.object;
            rhs.object = nullid;
        }
        return *this;
    }

private:
    HandleId object;
};

template <typename T>
struct Handle : public HandleBase {
    Handle() noexcept = default;

    Handle(const Handle& rhs) noexcept = default;
    Handle(Handle&& rhs) noexcept = default;

    Handle& operator=(const Handle& rhs) noexcept
    {
        HandleBase::operator=(rhs);
        return *this;
    }
    Handle& operator=(Handle&& rhs) noexcept
    {
        HandleBase::operator=(std::move(rhs));
        return *this;
    }

    explicit Handle(HandleId id) noexcept
        : HandleBase(id)
    {
    }

    template <typename B, typename = std::enable_if_t<std::is_base_of_v<HandleBase, B>>>
    Handle(const Handle<B>& base) noexcept
        : HandleBase(base)
    {
    }
};

using VertexBufferInfoHandle = Handle<RHIVertexBufferInfo>;
using VertexBufferHandle = Handle<RHIVertexBuffer>;
using IndexBufferHandle = Handle<RHIIndexBuffer>;
using TextureHandle = Handle<RHITexture>;
using ShaderModuleHandle = Handle<RHIShaderModule>;
using DescriptorSetLayoutHandle =  Handle<RHIDescriptorSetLayout>;
using DescriptorSetHandle = Handle<RHIDescriptorSet>;
using PipelineHandle = Handle<RHIPipeline>;
using SwapchainHandle = Handle<RHISwapchain>;

} // namespace rhi
} // namespace ocf
