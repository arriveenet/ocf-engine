#include <ocf/audio/AudioSystem.h>
#include <ocf/core/Engine.h>
#include <ocf/core/event/Events.h>
#include <ocf/math/geometric.h>
#include <ocf/platform/Application.h>
#include <ocf/platform/FileSystem.h>
#include <ocf/resource/FontManager.h>
#include <ocf/resource/Mesh.h>
#include <ocf/resource/ModelImporter.h>
#include <ocf/scene/Camera.h>
#include <ocf/scene/MeshInstance.h>
#include <ocf/scene/Node.h>
#include <ocf/scene/Scene.h>
#include <ocf/scene/SceneGraph.h>
#include <ocf/scene/SceneManager.h>
#include <ocf/scene/Text.h>
#include <ocf/scene/View.h>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <array>
#include <cmath>
#include <format>
#include <string_view>

using namespace ocf;

namespace {

constexpr int UI_LAYER = 100;
constexpr int PAUSE_LAYER = 200;

/**
 * Data shared by the scenes. main() owns it and it outlives every scene, so scenes may keep a
 * reference to it. It must not hold GPU resources, since it also outlives the engine.
 */
struct GameState {
    float modelRotation = 0.0f; // Degrees; written by MainScene, shown by HudScene
};

bool isKeyPressed(const Event& event, KeyCode key)
{
    if (event.getEventType() != EventType::KeyboardEvent) {
        return false;
    }
    const auto& keyboardEvent = static_cast<const KeyboardEvent&>(event);
    return keyboardEvent.isPressed() && keyboardEvent.getKeyCode() == key;
}

/**
 * Base for scenes made of screen-space text. The camera is in pixels with the origin at the
 * bottom left of the window.
 */
class UIScene : public Scene {
protected:
    void setupUI(View::ClearFlags clearFlags)
    {
        const math::vec2 size = getScreenSize();

        m_ui = createSceneGraph();
        Camera* camera = m_ui->getRoot()->createChild()->addComponent<Camera>();
        camera->orthographic(0.0f, size.x, 0.0f, size.y);

        View* view = createView();
        view->setScene(m_ui);
        view->setCamera(camera);
        view->setClearFlags(clearFlags);
        view->setClearColor(math::vec4(0.1f, 0.1f, 0.15f, 1.0f));
        view->setSortMode(View::SortMode::SubmitOrder);
    }

    Text* addText(const math::vec2& position, const math::vec2& anchor, std::string_view string,
                  int fontSize = 32)
    {
        FontTrueTypeConfig config;
        config.fontPath = "fonts/NotoSansJP-Regular.ttf";
        config.fontSize = fontSize;
        Ref<Font> font = getEngine().getFontManager().loadFontTTF(config);

        Node* node = m_ui->getRoot()->createChild();
        node->setPosition(math::vec3(position.x, position.y, 0.0f));

        Text* text = node->addComponent<Text>(getEngine(), font);
        text->setPixelsPerUnit(1.0f);
        text->setAnchor(anchor);
        text->setString(string);
        return text;
    }

    math::vec2 getScreenSize() const
    {
        const math::ivec2 size = getEngine().getWindowSize();
        return math::vec2(static_cast<float>(size.x), static_cast<float>(size.y));
    }

    SceneGraph* m_ui = nullptr;
};

/** Pauses the scenes below it and takes all input until P is pressed again. */
class PauseScene : public UIScene {
public:
    PauseScene()
    {
        setBlocksUpdate(true);
        setBlocksInput(true);
    }

    void onEnter() override
    {
        setupUI(View::ClearFlags::Depth);
        addText(getScreenSize() * 0.5f, math::vec2(0.5f, 0.5f), "PAUSED - Press P to resume");
    }

    void onEvent(Event& event) override
    {
        if (isKeyPressed(event, KeyCode::P)) {
            close();
            event.setHandled(true);
        }
    }
};

/** Child of MainScene, drawn over it. */
class HudScene : public UIScene {
public:
    explicit HudScene(const GameState& state)
        : m_state(state)
    {
    }

    void onEnter() override
    {
        setupUI(View::ClearFlags::Depth);

        const math::vec2 size = getScreenSize();
        m_fpsText = addText(math::vec2(16.0f, size.y - 16.0f), math::vec2(0.0f, 0.0f), "");
        m_rotationText = addText(math::vec2(16.0f, size.y - 56.0f), math::vec2(0.0f, 0.0f), "");
        addText(math::vec2(16.0f, 16.0f), math::vec2(0.0f, 1.0f),
                "P: Pause   R / Space / S: Play / Pause / Stop audio   Up / Down: Volume", 16);
    }

    void onUpdate(float /*deltaTime*/) override
    {
        m_fpsText->setString(
            std::format("FPS: {:.1f}", getEngine().getFrameCounter().getFrameRate()));
        m_rotationText->setString(std::format("Rotation: {:.0f} deg", m_state.modelRotation));
    }

private:
    const GameState& m_state;
    Text* m_fpsText = nullptr;
    Text* m_rotationText = nullptr;
};

class MainScene : public Scene {
public:
    explicit MainScene(GameState& state)
        : m_state(state)
    {
    }

    // Runs on a worker thread while the loading screen is shown: CPU work only
    void onLoad() override
    {
        ModelImporter importer;
        auto result =
            importer.loadFromFile("models/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf");
        if (result.isOk()) {
            m_mesh = result.unwrap();
        }
        setLoadProgress(0.9f);

        m_world = createSceneGraph();
        m_camera = m_world->getRoot()->createChild()->addComponent<Camera>();

        // Two copies of the model, so that each draw must get its own model matrix
        for (Node*& node : m_modelNodes) {
            node = m_world->getRoot()->createChild();
        }
        m_modelNodes[1]->setPosition(math::vec3(0.0f, 0.0f, -8.0f));
        m_modelNodes[1]->setScale(math::vec3(0.5f));
    }

    // Main thread: GPU resources
    void onEnter() override
    {
        Engine& engine = getEngine();
        const math::ivec2 size = engine.getWindowSize();
        m_camera->perspective(math::radians(60.0f),
                              static_cast<float>(size.x) / static_cast<float>(size.y), 0.1f,
                              100.0f);
        m_camera->lookAt(math::vec3(0.0f, 6.0f, 10.0f), math::vec3(0.0f, 0.0f, 0.0f),
                         math::vec3(0.0f, 1.0f, 0.0f));

        View* view = createView();
        view->setScene(m_world);
        view->setCamera(m_camera);

        if (m_mesh.ptr() != nullptr) {
            m_mesh->createSubMeshBuffers(engine);

            FileSystem* fileSystem = FileSystem::getInstance();
            const auto vsPath = fileSystem->getAssetFullPath("shaders/basic_pbr.vert.spv");
            const auto fsPath = fileSystem->getAssetFullPath("shaders/basic_pbr.frag.spv");
            for (Node* node : m_modelNodes) {
                node->addComponent<MeshInstance>(engine, vsPath, fsPath)->setMesh(m_mesh);
            }
        }

        auto& audioSystem = engine.getAudioSystem();
        m_audioBuffer = audioSystem.createStreamBuffer("audio/Canon in D Major.mp3");
        m_audioSource = audioSystem.createSource(m_audioBuffer);

        addChild<HudScene>(UI_LAYER, m_state);
    }

    void onExit() override
    {
        auto& audioSystem = getEngine().getAudioSystem();
        audioSystem.stop(m_audioSource);
        audioSystem.destroySource(m_audioSource);
        audioSystem.destroyBuffer(m_audioBuffer);

        if (m_mesh.ptr() != nullptr) {
            m_mesh->terminate(getEngine());
        }
    }

    void onUpdate(float deltaTime) override
    {
        m_state.modelRotation = std::fmod(m_state.modelRotation + 16.0f * deltaTime, 360.0f);
        m_modelNodes[0]->setRotation(math::vec3(0.0f, m_state.modelRotation, 0.0f));
        m_modelNodes[1]->setRotation(math::vec3(0.0f, -m_state.modelRotation, 0.0f));
    }

    void onEvent(Event& event) override
    {
        auto& audioSystem = getEngine().getAudioSystem();

        if (isKeyPressed(event, KeyCode::P)) {
            getSceneManager().add<PauseScene>(PAUSE_LAYER);
        } else if (isKeyPressed(event, KeyCode::R)) {
            audioSystem.play(m_audioSource);
        } else if (isKeyPressed(event, KeyCode::Space)) {
            audioSystem.pause(m_audioSource);
        } else if (isKeyPressed(event, KeyCode::S)) {
            audioSystem.stop(m_audioSource);
        } else if (isKeyPressed(event, KeyCode::Up)) {
            const float volume = audioSystem.getVolume(m_audioSource) + 0.1f;
            audioSystem.setVolume(m_audioSource, volume > 1.0f ? 1.0f : volume);
        } else if (isKeyPressed(event, KeyCode::Down)) {
            const float volume = audioSystem.getVolume(m_audioSource) - 0.1f;
            audioSystem.setVolume(m_audioSource, volume < 0.0f ? 0.0f : volume);
        } else {
            return;
        }
        event.setHandled(true);
    }

private:
    GameState& m_state;
    Ref<Mesh> m_mesh;
    SceneGraph* m_world = nullptr;
    Camera* m_camera = nullptr;
    std::array<Node*, 2> m_modelNodes{};
    audio::AudioBufferHandle m_audioBuffer;
    audio::AudioSourceHandle m_audioSource;
};

/** Loads MainScene in the background, then switches to it. */
class LoadingScene : public UIScene {
public:
    // Keep the loading screen up for a moment even when loading is quick
    static constexpr float MIN_DISPLAY_TIME = 1.0f;

    explicit LoadingScene(GameState& state)
        : m_state(state)
    {
    }

    void onEnter() override
    {
        setupUI(View::ClearFlags::All);
        m_progressText = addText(getScreenSize() * 0.5f, math::vec2(0.5f, 0.5f), "Loading...");

        m_operation = getSceneManager().loadAsync<MainScene>(SceneManager::LoadMode::Single, 0,
                                                             m_state);
        m_operation.setAllowActivation(false);
    }

    void onUpdate(float deltaTime) override
    {
        m_elapsed += deltaTime;
        m_progressText->setString(
            std::format("Loading... {:.0f}%", m_operation.progress() * 100.0f));

        // Single mode: activating MainScene removes this scene
        if (m_operation.isLoaded() && m_elapsed >= MIN_DISPLAY_TIME) {
            m_operation.activate();
        }
    }

private:
    GameState& m_state;
    SceneLoadOperation m_operation;
    Text* m_progressText = nullptr;
    float m_elapsed = 0.0f;
};

class TitleScene : public UIScene {
public:
    explicit TitleScene(GameState& state)
        : m_state(state)
    {
    }

    void onEnter() override
    {
        setupUI(View::ClearFlags::All);

        const math::vec2 center = getScreenSize() * 0.5f;
        addText(center + math::vec2(0.0f, 40.0f), math::vec2(0.5f, 0.5f), "OCF Engine Sandbox");
        addText(center - math::vec2(0.0f, 40.0f), math::vec2(0.5f, 0.5f), "Press Enter to start",
                24);
    }

    void onEvent(Event& event) override
    {
        if (isKeyPressed(event, KeyCode::Enter)) {
            getSceneManager().change<LoadingScene>(m_state);
            event.setHandled(true);
        }
    }

private:
    GameState& m_state;
};

} // namespace

int main()
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    GameState state;

    Application::Config config;
    config.title = "SandBox Test";

    Application& app = Application::getInstance();
    app.run(
        config,
        [&state](Engine& engine) { engine.getSceneManager().change<TitleScene>(state); },
        nullptr);
}
