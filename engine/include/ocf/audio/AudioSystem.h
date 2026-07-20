#pragma once
#include <memory>

namespace ocf {
namespace audio {

class AudioDevice;
class AudioMixer;

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool initialize();

    void shutdown();

    bool isInitialized() const noexcept { return m_initialized; }

private:
    struct Imple;
    std::unique_ptr<Imple> m_imple = nullptr;
    bool m_initialized = false;
};

} // namespace audio
} // namespace ocf
