// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/core/event/EventDispatcher.h"
#include "ocf/core/job/Job.h"
#include "ocf/scene/Scene.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ocf {

class Engine;
class Event;
class View;

/**
 * @brief Handle to a scene being loaded by SceneManager::loadAsync().
 *
 * By default the scene is activated as soon as it is loaded. Disallow activation to keep it
 * waiting, e.g. to show a loading screen for a minimum time, then call activate().
 */
class OCF_API SceneLoadOperation {
public:
    SceneLoadOperation() = default;

    bool isValid() const noexcept { return m_state != nullptr; }

    /** @brief Progress of the load in [0, 1], updated once per frame. */
    float progress() const;

    /** @brief onLoad() has finished; the scene is waiting to be activated. */
    bool isLoaded() const;

    /** @brief The scene has been activated. */
    bool isDone() const;

    void setAllowActivation(bool allow);

    /** @brief Activate the scene at the start of the next update once it is loaded. */
    void activate() { setAllowActivation(true); }

private:
    friend class SceneManager;

    struct State;

    explicit SceneLoadOperation(std::shared_ptr<State> state);

    std::shared_ptr<State> m_state;
};

/**
 * @brief Runs the scenes of the application, ordered by layer.
 *
 * Every frame the scenes are updated from the bottom layer up, drawn from the bottom layer
 * up, and receive input from the top layer down. Within a layer, scenes added later are
 * above the earlier ones.
 *
 * Changes (change, add, remove, child scenes) are queued and applied at the start of the
 * next update, so they can be requested from any scene callback.
 */
class OCF_API SceneManager {
public:
    enum class LoadMode : uint8_t {
        Single,   //!< Remove the scenes that are not persistent, then add the scene
        Additive, //!< Add the scene on top of the current ones
    };

    /** @param engine Passed to the scenes; may be null when no scene uses it (tests). */
    explicit SceneManager(Engine* engine);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    /**
     * @brief Switch to a new scene on layer 0, removing every scene that is not persistent.
     * @return The scene, valid until it is removed
     */
    template <typename T, typename... Args>
    T* change(Args&&... args)
    {
        return enqueueNew<T>(0, true, std::forward<Args>(args)...);
    }

    /**
     * @brief Add a scene on top of the current ones, e.g. a pause menu or a debug overlay.
     * @return The scene, valid until it is removed
     */
    template <typename T, typename... Args>
    T* add(int layer, Args&&... args)
    {
        return enqueueNew<T>(layer, false, std::forward<Args>(args)...);
    }

    /** @brief Remove a scene and its children at the start of the next update. */
    void remove(Scene* scene);

    /**
     * @brief Load a scene in the background. Its onLoad() runs on a worker thread, and the
     * scene is activated on the main thread once loaded and allowed.
     */
    template <typename T, typename... Args>
    SceneLoadOperation loadAsync(LoadMode mode, int layer, Args&&... args)
    {
        static_assert(std::is_base_of_v<Scene, T>, "T must derive from Scene");
        return startLoad(std::make_unique<T>(std::forward<Args>(args)...), mode, layer);
    }

    /** @brief Apply the queued changes, then update the active scenes. */
    void update(float deltaTime);

    /**
     * @brief Let every active scene build its debug UI, from the bottom layer up.
     *
     * Only called by the engine while an ImGui frame is open.
     */
    void updateImGui();

    /** @brief Pass an input event to the active scenes from the top layer down. */
    void dispatchEvent(Event& event);

    /**
     * @brief Pass an event to every active scene from the top layer down.
     *
     * For notifications such as a window resize, which neither the handled flag nor
     * Scene::setBlocksInput() stops.
     */
    void broadcastEvent(Event& event);

    /** @brief Fill views with the views of the active scenes, in drawing order. */
    void collectViews(std::vector<View*>& views);

    /** @brief The scenes, sorted by layer from the bottom up. */
    const std::vector<std::unique_ptr<Scene>>& getScenes() const noexcept { return m_scenes; }

private:
    friend class Scene;

    struct PendingAdd {
        std::unique_ptr<Scene> scene;
        bool replace = false;
    };

    struct PendingLoad {
        std::unique_ptr<Scene> scene;
        LoadMode mode = LoadMode::Single;
        job::JobHandle job;
        std::shared_ptr<SceneLoadOperation::State> state;
    };

    struct ViewEntry {
        int layer = 0;
        int order = 0;
        View* view = nullptr;
    };

    template <typename T, typename... Args>
    T* enqueueNew(int layer, bool replace, Args&&... args)
    {
        static_assert(std::is_base_of_v<Scene, T>, "T must derive from Scene");
        auto scene = std::make_unique<T>(std::forward<Args>(args)...);
        T* scenePtr = scene.get();
        enqueue(std::move(scene), layer, nullptr, replace);
        return scenePtr;
    }

    void attach(Scene& scene, int layer, Scene* parent);
    void enqueue(std::unique_ptr<Scene> scene, int layer, Scene* parent, bool replace);
    SceneLoadOperation startLoad(std::unique_ptr<Scene> scene, LoadMode mode, int layer);

    void applyPending();
    void applyRemovals();
    void applyAdds();
    void pollLoads();

    Scene* insert(std::unique_ptr<Scene> scene);
    void activate(Scene* scene);
    void removeNow(Scene* scene);
    void removeNonPersistent();

    Engine* m_engine = nullptr;
    std::vector<std::unique_ptr<Scene>> m_scenes; // Sorted by layer
    std::vector<PendingAdd> m_pendingAdds;
    std::vector<PendingLoad> m_loads;
    bool m_hasPendingRemovals = false;
    std::vector<ViewEntry> m_viewEntries; // Scratch for collectViews()

    Subscription m_keyboardSubscription;
    Subscription m_mouseSubscription;
    Subscription m_resizeSubscription;
};

} // namespace ocf
