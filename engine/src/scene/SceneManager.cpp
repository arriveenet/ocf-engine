// SPDX-License-Identifier: MIT
#include "ocf/scene/SceneManager.h"

#include "ocf/core/Engine.h"
#include "ocf/core/event/KeyboardEvent.h"
#include "ocf/core/event/MouseEvent.h"
#include "ocf/core/job/JobSystem.h"
#include "ocf/scene/SceneGraph.h"
#include "ocf/scene/View.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>

namespace ocf {

struct SceneLoadOperation::State {
    std::atomic<bool> loaded{false}; // Set by the loading thread when onLoad() returns
    float progress = 0.0f;
    bool allowActivation = true;
    bool done = false;
};

SceneLoadOperation::SceneLoadOperation(std::shared_ptr<State> state)
    : m_state(std::move(state))
{
}

float SceneLoadOperation::progress() const
{
    return m_state ? m_state->progress : 0.0f;
}

bool SceneLoadOperation::isLoaded() const
{
    return m_state && m_state->loaded.load(std::memory_order_acquire);
}

bool SceneLoadOperation::isDone() const
{
    return m_state && m_state->done;
}

void SceneLoadOperation::setAllowActivation(bool allow)
{
    if (m_state) {
        m_state->allowActivation = allow;
    }
}

SceneManager::SceneManager(Engine* engine)
    : m_engine(engine)
{
    if (m_engine != nullptr) {
        EventDispatcher& dispatcher = m_engine->getEventDispatcher();
        m_keyboardSubscription = dispatcher.subscribe<KeyboardEvent>(
            [this](KeyboardEvent& event) { dispatchEvent(event); });
        m_mouseSubscription = dispatcher.subscribe<MouseEvent>(
            [this](MouseEvent& event) { dispatchEvent(event); });
    }
}

SceneManager::~SceneManager()
{
    // A scene that is still loading is in use by a worker thread
    auto& jobSystem = job::JobSystem::getInstance();
    for (PendingLoad& load : m_loads) {
        if (load.job.isValid()) {
            jobSystem.wait(load.job);
        }
        while (!load.state->loaded.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
    m_loads.clear();

    // Queued scenes were never added; drop them before the parents they may refer to
    m_pendingAdds.clear();

    // Remove from the top down, each top-level scene together with its children
    while (!m_scenes.empty()) {
        auto it = std::find_if(m_scenes.rbegin(), m_scenes.rend(),
                               [](const auto& scene) { return scene->m_parent == nullptr; });
        removeNow(it != m_scenes.rend() ? it->get() : m_scenes.back().get());
    }
}

void SceneManager::remove(Scene* scene)
{
    if (scene == nullptr || scene->m_sceneManager != this) {
        return;
    }

    scene->m_removeRequested = true;
    m_hasPendingRemovals = true;
}

void SceneManager::update(float deltaTime)
{
    applyPending();
    pollLoads();
    // Scenes activated by pollLoads() may have requested changes from onEnter()
    applyPending();

    // The scenes below the topmost one that blocks the update are paused
    size_t first = 0;
    for (size_t i = m_scenes.size(); i-- > 0;) {
        if (m_scenes[i]->isActive() && m_scenes[i]->m_blocksUpdate) {
            first = i;
            break;
        }
    }

    // m_scenes only changes in applyPending() and pollLoads(), never during this loop
    for (size_t i = first; i < m_scenes.size(); i++) {
        Scene* scene = m_scenes[i].get();
        if (!scene->isActive()) {
            continue;
        }

        scene->onUpdate(deltaTime);

        for (const auto& sceneGraph : scene->getSceneGraphs()) {
            sceneGraph->update(deltaTime);
        }
    }
}

void SceneManager::updateImGui()
{
    // m_scenes only changes in update(); requests from onImGui() are applied there
    for (const auto& scene : m_scenes) {
        if (scene->isActive()) {
            scene->onImGui();
        }
    }
}

void SceneManager::dispatchEvent(Event& event)
{
    for (size_t i = m_scenes.size(); i-- > 0;) {
        Scene* scene = m_scenes[i].get();
        if (!scene->isActive()) {
            continue;
        }

        scene->onEvent(event);

        if (event.isHandled() || scene->m_blocksInput) {
            break;
        }
    }
}

void SceneManager::collectViews(std::vector<View*>& views)
{
    m_viewEntries.clear();
    for (const auto& scene : m_scenes) {
        if (!scene->isActive()) {
            continue;
        }

        for (const auto& view : scene->getViews()) {
            m_viewEntries.push_back({scene->m_layer, view->getOrder(), view.get()});
        }
    }

    std::stable_sort(m_viewEntries.begin(), m_viewEntries.end(),
                     [](const ViewEntry& a, const ViewEntry& b) {
                         if (a.layer != b.layer) {
                             return a.layer < b.layer;
                         }
                         return a.order < b.order;
                     });

    views.clear();
    for (const ViewEntry& entry : m_viewEntries) {
        views.push_back(entry.view);
    }
}

void SceneManager::attach(Scene& scene, int layer, Scene* parent)
{
    scene.m_engine = m_engine;
    scene.m_sceneManager = this;
    scene.m_layer = layer;
    scene.m_parent = parent;
}

void SceneManager::enqueue(std::unique_ptr<Scene> scene, int layer, Scene* parent, bool replace)
{
    attach(*scene, layer, parent);
    m_pendingAdds.push_back({std::move(scene), replace});
}

SceneLoadOperation SceneManager::startLoad(std::unique_ptr<Scene> scene, LoadMode mode, int layer)
{
    attach(*scene, layer, nullptr);
    scene->m_state = Scene::State::Loading;

    auto state = std::make_shared<SceneLoadOperation::State>();

    // The scene is owned by m_loads and does not move, so the job can keep a pointer to it
    Scene* loadingScene = scene.get();
    auto load = [loadingScene, state](void*) {
        loadingScene->onLoad();
        state->loaded.store(true, std::memory_order_release);
    };

    job::JobHandle handle;
    auto& jobSystem = job::JobSystem::getInstance();
    if (jobSystem.isInitialized()) {
        handle = jobSystem.createJob(load);
    }

    if (handle.isValid()) {
        jobSystem.run(handle);
    } else {
        // No worker threads available; load on this thread
        load(nullptr);
    }

    m_loads.push_back({std::move(scene), mode, handle, state});
    return SceneLoadOperation(std::move(state));
}

void SceneManager::applyPending()
{
    // Callbacks run here may request more changes; apply until none are left
    while (m_hasPendingRemovals || !m_pendingAdds.empty()) {
        applyRemovals();
        applyAdds();
    }
}

void SceneManager::applyRemovals()
{
    m_hasPendingRemovals = false;

    for (;;) {
        auto it = std::find_if(m_scenes.rbegin(), m_scenes.rend(),
                               [](const auto& scene) { return scene->m_removeRequested; });
        if (it == m_scenes.rend()) {
            break;
        }
        removeNow(it->get());
    }
}

void SceneManager::applyAdds()
{
    // Indexed loop: callbacks may queue more scenes, which are applied in this same pass
    for (size_t i = 0; i < m_pendingAdds.size(); i++) {
        Scene* scene = m_pendingAdds[i].scene.get();
        const bool replace = m_pendingAdds[i].replace;

        // Removed before it was added, or its parent is going away. Removing a parent marks
        // its queued children, so m_parent is only read while the parent is alive.
        if (scene->m_removeRequested ||
            (scene->m_parent != nullptr && scene->m_parent->m_removeRequested)) {
            scene->m_removeRequested = true;
            continue;
        }

        std::unique_ptr<Scene> owned = std::move(m_pendingAdds[i].scene);

        if (replace) {
            removeNonPersistent();
        }

        owned->m_state = Scene::State::Loading;
        owned->onLoad();
        owned->m_state = Scene::State::Loaded;

        activate(insert(std::move(owned)));
    }

    m_pendingAdds.clear();
}

void SceneManager::pollLoads()
{
    for (size_t i = 0; i < m_loads.size();) {
        PendingLoad& load = m_loads[i];

        const bool loaded = load.state->loaded.load(std::memory_order_acquire);
        load.state->progress = loaded ? 1.0f : load.scene->getLoadProgress();
        if (!loaded) {
            i++;
            continue;
        }

        load.scene->m_state = Scene::State::Loaded;
        if (!load.state->allowActivation) {
            i++;
            continue;
        }

        // Take the load out first: onEnter() may start another one
        std::unique_ptr<Scene> scene = std::move(load.scene);
        const LoadMode mode = load.mode;
        std::shared_ptr<SceneLoadOperation::State> state = std::move(load.state);
        m_loads.erase(m_loads.begin() + static_cast<std::ptrdiff_t>(i));

        if (mode == LoadMode::Single) {
            removeNonPersistent();
        }

        activate(insert(std::move(scene)));
        state->done = true;
    }
}

Scene* SceneManager::insert(std::unique_ptr<Scene> scene)
{
    // Above the scenes already on the same layer
    auto it = std::upper_bound(m_scenes.begin(), m_scenes.end(), scene->m_layer,
                               [](int layer, const auto& other) { return layer < other->m_layer; });
    return m_scenes.insert(it, std::move(scene))->get();
}

void SceneManager::activate(Scene* scene)
{
    scene->onEnter();
    scene->m_state = Scene::State::Active;
}

void SceneManager::removeNow(Scene* scene)
{
    if (scene->m_state == Scene::State::Active) {
        scene->onExit();
    }
    scene->m_state = Scene::State::Exited;

    // Children never outlive their parent; remove them from the top down
    for (;;) {
        auto child = std::find_if(m_scenes.rbegin(), m_scenes.rend(),
                                  [scene](const auto& other) { return other->m_parent == scene; });
        if (child == m_scenes.rend()) {
            break;
        }
        removeNow(child->get());
    }

    // Queued children will never be added
    for (PendingAdd& add : m_pendingAdds) {
        if (add.scene != nullptr && add.scene->m_parent == scene) {
            add.scene->m_removeRequested = true;
        }
    }

    auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
                           [scene](const auto& other) { return other.get() == scene; });
    if (it != m_scenes.end()) {
        m_scenes.erase(it);
    }
}

void SceneManager::removeNonPersistent()
{
    for (;;) {
        auto it = std::find_if(m_scenes.rbegin(), m_scenes.rend(), [](const auto& scene) {
            return scene->m_parent == nullptr && !scene->m_persistent;
        });
        if (it == m_scenes.rend()) {
            break;
        }
        removeNow(it->get());
    }
}

} // namespace ocf
