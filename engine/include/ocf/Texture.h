#pragma once
#include "ocf/RendererEnums.h"
#include <stdint.h>

namespace ocf {

class Texture {
    struct BuilderDetails;

public:
    class Builder {
    public:
        Builder();
        ~Builder();

        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Builder& width(uint32_t width) noexcept;
        Builder& height(uint32_t height) noexcept;
        Builder& depth(uint32_t depth) noexcept;
        Builder& levels(uint8_t levels) noexcept;
        Builder& sampler(SamplerType sampler) noexcept;
        Builder& format(TextureFormat format) noexcept;

        Texture* build();

    private:
        BuilderDetails* m_impl = nullptr;
    };

    Texture(const Builder& builder);
    ~Texture();

    void setImage(uint32_t level, PixelDataFormat format, PixelDataType type,
                  const void* data) noexcept;

    void generateMipmaps() noexcept;

    unsigned int getHandle() const noexcept { return m_handle; }

    uint32_t getWidth() const noexcept { return m_width; }

    uint32_t getHeight() const noexcept { return m_height; }

    uint32_t getDepth() const noexcept { return m_depth; }

    uint8_t getLevels() const noexcept { return m_levels; }

    SamplerType getSamplerType() const noexcept { return m_samplerType; }

    TextureFormat getFormat() const noexcept { return m_format; }

protected:
    unsigned int m_handle = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_depth = 1;
    uint8_t m_levels = 1;
    SamplerType m_samplerType = SamplerType::SAMPLER_2D;
    TextureFormat m_format = TextureFormat::RGBA8;
};

} // namespace ocf
