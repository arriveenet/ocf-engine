// SPDX - License - Identifier : MIT

#pragma once

#include <cstdint>

namespace ocf {
namespace rhi {

struct RHITexture;
struct RHIVertexBuffer;
struct RHISwapchain;

template <typename T>
struct Handle {
public:
    using HandleId = uint32_t;
    static constexpr HandleId nullid = UINT32_MAX;

    Handle() : m_id(nullid) {}
    explicit Handle(HandleId id) : m_id(id) {}

    explicit operator bool() const noexcept { return m_id != nullid; }

    HandleId getId() const noexcept { return m_id; }

private:
    HandleId m_id;
};

using TextureHandle = Handle<RHITexture>;
using VertexBufferHandle = Handle<RHIVertexBuffer>;
using SwapchainHandle = Handle<RHISwapchain>;

} // namespace rhi
} // namespace ocf
