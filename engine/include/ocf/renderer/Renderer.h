// SPDX-License-Identifier: MIT

#pragma once

namespace ocf {

namespace rhi {
class Device;
}

class Renderer {
public:
    explicit Renderer(rhi::Device* device);
    ~Renderer();

    void beginFrame();

    void endFrame();

    void render();

private:
    rhi::Device* m_device = nullptr;
};

} // namespace ocf
