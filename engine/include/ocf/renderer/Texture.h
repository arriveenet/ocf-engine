// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/rhi/PixelBufferDescriptor.h"
#include "ocf/rhi/RHIEnums.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ocf {

class Engine;

class Texture {
    struct BuilderDetails;

public:
    static constexpr size_t BASE_LEVEL = 0;

    using TextureHandle = rhi::TextureHandle;
    using Format = rhi::PixelDataFormat;
    using Type = rhi::PixelDataType;
    using InternalFormat = rhi::TextureFormat;
    using Sampler = rhi::SamplerType;
    using PixelBufferDescriptor = rhi::PixelBufferDescriptor;

    class Builder {
    public:
        Builder();
        ~Builder();
        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& width(uint32_t width);
        Builder& height(uint32_t height);
        Builder& depth(uint32_t depth);
        Builder& levels(uint8_t levels);
        Builder& format(InternalFormat format);
        Builder& sampler(Sampler sampler);

        Texture* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    static size_t valueForLevel(uint8_t const level, size_t const baseLevelValue) noexcept
    {
        return (std::max)(size_t(1), baseLevelValue >> level);
    }

    static uint8_t calculateMipLevels(uint32_t maxDimension) noexcept
    {
        return static_cast<uint8_t>(std::max(std::ilogbf(static_cast<float>(maxDimension)) + 1, 1));
    }

    static uint8_t calculateMipLevels(uint32_t width, uint32_t height) noexcept
    {
        const uint32_t maxDimension = std::max(width, height);
        return calculateMipLevels(maxDimension);
    }

    Texture(Engine& engine, const Builder& builder);

    void terminate(Engine& engine);

    void generateMipmaps(Engine& engine);

    TextureHandle getHandle() const noexcept { return m_handle; }

    size_t getWidth(size_t level = BASE_LEVEL) const;

    size_t getHeight(size_t level = BASE_LEVEL) const;

    size_t getDepth(size_t level = BASE_LEVEL) const;

    uint8_t getLevels() const { return m_levelCount; }

    InternalFormat getFormat() const { return m_format; }

    Sampler getSampler() const { return m_sampler; }

    void setImage(Engine& engine, size_t level, uint32_t xoffset, uint32_t yoffset, uint32_t zoffset,
                  uint32_t width, uint32_t height, uint32_t depth,
                  PixelBufferDescriptor&& buffer) const;

    void setImage(Engine& engine, size_t level, PixelBufferDescriptor&& buffer) const
    {
        setImage(engine, level, 0, 0, 0, static_cast<uint32_t>(getWidth(level)),
                 static_cast<uint32_t>(getHeight(level)), m_depth, std::move(buffer));
    }

private:
    TextureHandle m_handle;
    uint32_t m_width = 1;
    uint32_t m_height = 1;
    uint32_t m_depth = 1;
    uint8_t m_levelCount = 1;
    InternalFormat m_format = InternalFormat::RGBA8;
    Sampler m_sampler = Sampler::Sampler2D;
};

} // namespace ocf
