#include <ocf/audio/AudioSystem.h>
#include <ocf/core/Engine.h>
#include <ocf/core/event/Events.h>
#include <ocf/platform/Application.h>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif
#include <ocf/core/event/Event.h>

using namespace ocf;

void setup(Engine& engine, Scene *scene) {
    auto& audioSystem = engine.getAudioSystem();
    auto bufferHandle = audioSystem.createStreamBuffer("audio/Canon in D Major.mp3");
    auto sourceHandle = audioSystem.createSource(bufferHandle);

    auto& eventDispatcher = engine.getEventDispatcher();
    eventDispatcher.subscribe<KeyboardEvent>([sourceHandle, &audioSystem](const KeyboardEvent& event) {
            if (event.isPressed() && event.getKeyCode() == KeyCode::Space) {
                audioSystem.pause(sourceHandle);
            }
            if (event.isPressed() && event.getKeyCode() == KeyCode::P) {
                audioSystem.play(sourceHandle);
            }
            if (event.isPressed() && event.getKeyCode() == KeyCode::S) {
                audioSystem.stop(sourceHandle);
            }

            if (event.isPressed() && event.getKeyCode() == KeyCode::Up) {
                float volume = audioSystem.getVolume(sourceHandle);
                volume += 0.1f;
                if (volume > 1.0f) volume = 1.0f;
                audioSystem.setVolume(sourceHandle, volume);
            }
            if (event.isPressed() && event.getKeyCode() == KeyCode::Down) {
                float volume = audioSystem.getVolume(sourceHandle);
                volume -= 0.1f;
                if (volume < 0.0f) volume = 0.0f;
                audioSystem.setVolume(sourceHandle, volume);
            }
        });

}

void cleanup(Engine& engine, Scene *scene) {}

int main()
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Application::Config config;
    config.title = "SandBox Test";

    Application& app = Application::getInstance();
    app.run(config, setup, cleanup);

}
