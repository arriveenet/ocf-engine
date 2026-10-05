// SPDX-License-Identifier: MIT
#pragma once

namespace ocf {
namespace audio {

class AudioMixer;

class AudioDevice {
public:
    virtual ~AudioDevice() = default;

    virtual bool initialize(AudioMixer* mixer) = 0;

    virtual void shutdown() = 0;

    virtual bool start() = 0;

    virtual void stop() = 0;
};

} // namespace audio
} // namespace ocf
