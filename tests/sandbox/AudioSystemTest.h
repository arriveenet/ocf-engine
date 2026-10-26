// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/audio/AudioEnums.h>

#include <string>

class AudioSystemTest : public TestBaseScene {
public:
    void onEnter() override;
    void onExit() override;
    void onImGui() override;

private:
    ocf::audio::AudioBufferHandle m_audioBuffer;
    ocf::audio::AudioSourceHandle m_audioSource;
    float m_volume = 1.0f;
    std::string m_audioFilePath = "audio/Canon in D Major.mp3";
};
