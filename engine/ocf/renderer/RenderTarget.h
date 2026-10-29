// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/rhi/Handle.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ocf {

class Engine;
class Texture;

/**
 * @brief Textures that a View draws into instead of the swapchain.
 *
 * The textures are referenced, not owned, and must be built with the attachment usage
 * (Texture::Usage::ColorAttachment or DepthAttachment). A texture that is also Sampleable can be
 * used by materials once the view that draws into it has been rendered, i.e. by views with a
 * higher order. Either attachment may be left out, e.g. a depth-only target for a shadow map.
 */
class OCF_API RenderTarget {
    struct BuilderDetails;

public:
    enum class AttachmentPoint : uint8_t {
        Color,
        Depth,
    };

    static constexpr size_t ATTACHMENT_COUNT = 2;

    class Builder {
    public:
        Builder();
        ~Builder();
        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& texture(AttachmentPoint attachment, Texture* texture);

        /** @brief The size is that of the textures, which must all be the same size. */
        RenderTarget* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    RenderTarget(Engine& engine, const Builder& builder);

    /** @brief Destroy the render target. The textures are not destroyed. */
    void terminate(Engine& engine);

    Texture* getTexture(AttachmentPoint attachment) const noexcept
    {
        return m_textures[static_cast<size_t>(attachment)];
    }

    uint32_t getWidth() const noexcept { return m_width; }

    uint32_t getHeight() const noexcept { return m_height; }

    rhi::RenderTargetHandle getHandle() const noexcept { return m_handle; }

private:
    rhi::RenderTargetHandle m_handle;
    std::array<Texture*, ATTACHMENT_COUNT> m_textures{};
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace ocf
