// SPDX-License-Identifier: MIT
#include "AudioSystemTest.h"

#include <ocf/audio/AudioSystem.h>
#include <ocf/core/Engine.h>
#include <ocf/scene/View.h>

#include <imgui.h>

using namespace ocf;

void AudioSystemTest::onEnter()
{
    initializeScene();

    auto& audioSystem = getEngine().getAudioSystem();
    m_audioBuffer = audioSystem.createStreamBuffer(m_audioFilePath);
    m_audioSource = audioSystem.createSource(m_audioBuffer);
}

void AudioSystemTest::onExit()
{
    auto& audioSystem = getEngine().getAudioSystem();
    audioSystem.stop(m_audioSource);
    audioSystem.destroySource(m_audioSource);
    audioSystem.destroyBuffer(m_audioBuffer);
}

void AudioSystemTest::onImGui()
{
    auto& audioSystem = getEngine().getAudioSystem();

    ImGui::Begin("AudioSystemTest");

    ImGui::Text("Audio File: %s", m_audioFilePath.c_str());

    if (ImGui::Button("Play")) {
        audioSystem.play(m_audioSource);
    }

    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        audioSystem.pause(m_audioSource);
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        audioSystem.stop(m_audioSource);
    }

    bool isLooping = audioSystem.isLoop(m_audioSource);
    if (ImGui::Checkbox("Loop", &isLooping)) {
        audioSystem.setLoop(m_audioSource, isLooping);
    }

    if (ImGui::SliderFloat("Volume", &m_volume, 0.0f, 1.0f)) {
        audioSystem.setVolume(m_audioSource, m_volume);
    }

    ImGui::End();
}
