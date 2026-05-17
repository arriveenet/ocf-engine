#pragma once

#include "ocf/rhi/Handle.h"
#include "ocf/rhi/RHIEnums.h"

namespace ocf {

class Engine;

class Texture {
    struct BuilderDetails;

public:
    static constexpr size_t BASE_LEVEL = 0;

    using TextureHandle = rhi::TextureHandle;

    class Builder {
    public:
        Builder();
        ~Builder();
        BuilderDetails* operator->() noexcept { return m_impl; }
        const BuilderDetails* operator->() const noexcept { return m_impl; }

        Texture* build(Engine& engine);

    private:
        BuilderDetails* m_impl = nullptr;
    };

    Texture(Engine& engine, const Builder& builder);

    void terminate(Engine& engine);

    TextureHandle getHandle() const noexcept { return m_handle; }

    void setImage(uint32_t width, uint32_t height);

private:
    TextureHandle m_handle;
    uint32_t m_width = 1;
    uint32_t m_height = 1;
    uint8_t m_levelCount = 1;
    uint32_t m_depth = 1;
};

} // namespace ocf
