// SPDX-License-Identifier: MIT
#include "ocf/renderer/RenderTarget.h"

#include "ocf/core/Engine.h"
#include "ocf/renderer/Texture.h"
#include "ocf/rhi/Device.h"

#include <cassert>

namespace ocf {

struct RenderTarget::BuilderDetails {
    std::array<Texture*, ATTACHMENT_COUNT> textures{};
};

RenderTarget::Builder::Builder()
{
    m_impl = new BuilderDetails();
}

RenderTarget::Builder::~Builder()
{
    delete m_impl;
}

RenderTarget::Builder& RenderTarget::Builder::texture(AttachmentPoint attachment, Texture* texture)
{
    m_impl->textures[static_cast<size_t>(attachment)] = texture;
    return *this;
}

RenderTarget* RenderTarget::Builder::build(Engine& engine)
{
    const Texture* color = m_impl->textures[static_cast<size_t>(AttachmentPoint::Color)];
    const Texture* depth = m_impl->textures[static_cast<size_t>(AttachmentPoint::Depth)];
    assert((color != nullptr || depth != nullptr) && "RenderTarget needs at least one texture");
    assert((color == nullptr || (color->getUsage() & Texture::Usage::ColorAttachment)) &&
           "The color texture needs the ColorAttachment usage");
    assert((depth == nullptr || (depth->getUsage() & Texture::Usage::DepthAttachment)) &&
           "The depth texture needs the DepthAttachment usage");
    assert((color == nullptr || depth == nullptr ||
            (color->getWidth() == depth->getWidth() && color->getHeight() == depth->getHeight())) &&
           "The textures of a RenderTarget must be the same size");

    return new RenderTarget(engine, *this);
}

RenderTarget::RenderTarget(Engine& engine, const Builder& builder)
    : m_textures(builder->textures)
{
    rhi::TargetBufferInfo targets[ATTACHMENT_COUNT];
    for (size_t i = 0; i < ATTACHMENT_COUNT; i++) {
        if (const Texture* texture = m_textures[i]) {
            targets[i].handle = texture->getHandle();
            m_width = static_cast<uint32_t>(texture->getWidth());
            m_height = static_cast<uint32_t>(texture->getHeight());
        }
    }

    m_handle = engine.getDevice().createRenderTarget(
        m_width, m_height, targets[static_cast<size_t>(AttachmentPoint::Color)],
        targets[static_cast<size_t>(AttachmentPoint::Depth)]);
}

void RenderTarget::terminate(Engine& engine)
{
    engine.getDevice().destroyRenderTarget(m_handle);
}

} // namespace ocf
