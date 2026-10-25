// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ocf {

class Engine;
class Event;
class SceneGraph;
class SceneManager;
class View;

/**
 * @brief Unit of lifetime, loading, update and input, such as a title screen, a level or a HUD.
 *
 * A scene owns its scene graphs and views. The SceneManager runs several scenes at once,
 * ordered by layer: a higher layer is updated and drawn after, and receives input before, the
 * layers below it.
 *
 * Lifecycle: onLoad() -> onEnter() -> onUpdate() / onEvent() / onImGui() every frame -> onExit().
 *
 * onLoad() runs on a worker thread when the scene is loaded with SceneManager::loadAsync(),
 * so it must only do CPU work: reading files, parsing, building nodes. GPU resources are
 * created in onEnter(), which always runs on the main thread.
 *
 * getEngine() and getSceneManager() are valid once the scene has been handed to the scene
 * manager, i.e. not in the constructor.
 */
class OCF_API Scene {
public:
    enum class State : uint8_t {
        Created, //!< Handed to the scene manager, onLoad() not run yet
        Loading, //!< onLoad() is running
        Loaded,  //!< onLoad() is done, waiting to be activated
        Active,  //!< onEnter() is done; updated, drawn and receiving input
        Exited,  //!< onExit() is done, about to be destroyed
    };

    Scene();
    virtual ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    /** @brief Load data. May run on a worker thread: no GPU work, no scene manager calls. */
    virtual void onLoad() {}

    /** @brief Called on the main thread when the scene becomes active. */
    virtual void onEnter() {}

    /** @brief Called before the scene and its children are destroyed. */
    virtual void onExit() {}

    /** @brief Called every frame before the scene graphs are updated. */
    virtual void onUpdate(float /*deltaTime*/) {}

    /**
     * @brief Called for input events, from the top layer down.
     *
     * Call event.setHandled(true) to stop the event from reaching the layers below.
     */
    virtual void onEvent(Event& /*event*/) {}

    /**
     * @brief Called every frame after the update to build the Dear ImGui debug UI.
     *
     * Use the ImGui API only from here. When the engine is built without OCF_ENABLE_IMGUI,
     * ImGui is never initialized and this is never called, so the code needs no #if.
     *
     * Unlike onUpdate(), it is also called while a scene above blocks the update, so that the
     * debug UI of a paused scene stays visible.
     */
    virtual void onImGui() {}

    /**
     * @brief Add a scene whose lifetime is nested in this one.
     *
     * The child is created after and destroyed before this scene, so the returned pointer
     * stays valid for this scene's lifetime (but not in its destructor). Like every scene
     * manager change, the child is added at the start of the next update.
     */
    template <typename T, typename... Args>
    T* addChild(int layer, Args&&... args)
    {
        static_assert(std::is_base_of_v<Scene, T>, "T must derive from Scene");
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T* childPtr = child.get();
        addChildScene(layer, std::move(child));
        return childPtr;
    }

    /** @brief Remove this scene (and its children) at the start of the next update. */
    void close();

    SceneGraph* createSceneGraph();

    View* createView();

    const std::vector<std::unique_ptr<SceneGraph>>& getSceneGraphs() const noexcept { return m_sceneGraphs; }

    const std::vector<std::unique_ptr<View>>& getViews() const noexcept { return m_views; }

    Engine& getEngine() const { return *m_engine; }

    SceneManager& getSceneManager() const { return *m_sceneManager; }

    Scene* getParent() const noexcept { return m_parent; }

    int getLayer() const noexcept { return m_layer; }

    State getState() const noexcept { return m_state; }

    bool isActive() const noexcept { return m_state == State::Active; }

    /** @brief Stop the update of the layers below while this scene is active (e.g. pause). */
    void setBlocksUpdate(bool blocks) { m_blocksUpdate = blocks; }

    bool blocksUpdate() const noexcept { return m_blocksUpdate; }

    /** @brief Stop input from reaching the layers below (e.g. a modal menu). */
    void setBlocksInput(bool blocks) { m_blocksInput = blocks; }

    bool blocksInput() const noexcept { return m_blocksInput; }

    /** @brief Keep this scene when SceneManager::change() switches scenes (e.g. a fade). */
    void setPersistent(bool persistent) { m_persistent = persistent; }

    bool isPersistent() const noexcept { return m_persistent; }

    /** @brief Report the progress of onLoad() in [0, 1]. Callable from the loading thread. */
    void setLoadProgress(float progress) { m_loadProgress.store(progress, std::memory_order_relaxed); }

    float getLoadProgress() const noexcept { return m_loadProgress.load(std::memory_order_relaxed); }

private:
    friend class SceneManager;

    void addChildScene(int layer, std::unique_ptr<Scene> child);

    Engine* m_engine = nullptr;
    SceneManager* m_sceneManager = nullptr;
    Scene* m_parent = nullptr;
    int m_layer = 0;
    State m_state = State::Created;
    bool m_blocksUpdate = false;
    bool m_blocksInput = false;
    bool m_persistent = false;
    bool m_removeRequested = false;
    std::atomic<float> m_loadProgress{0.0f};

    // Views reference the scene graphs, so they are declared after them to be destroyed first
    std::vector<std::unique_ptr<SceneGraph>> m_sceneGraphs;
    std::vector<std::unique_ptr<View>> m_views;
};

} // namespace ocf
