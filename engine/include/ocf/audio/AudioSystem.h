#pragma once

#include "ocf/audio/AudioEnums.h"

#include <memory>
#include <string_view>

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

    void update();

    AudioHandle load(std::string_view filename);

    void play(AudioHandle handle);

    bool isInitialized() const noexcept { return m_initialized; }

private:
    struct Imple;
    std::unique_ptr<Imple> m_imple = nullptr;
    bool m_initialized = false;
};

} // namespace audio
} // namespace ocf
