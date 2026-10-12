// SPDX-License-Identifier: MIT
#include "ocf/core/event/Event.h"
#include "ocf/core/job/JobSystem.h"
#include "ocf/scene/Scene.h"
#include "ocf/scene/SceneManager.h"
#include "ocf/scene/View.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace ocf;

namespace {

using Log = std::vector<std::string>;

class TestEvent : public Event {
public:
    TestEvent()
        : Event(EventType::Custom)
    {
    }
};

class RecordingScene : public Scene {
public:
    RecordingScene(Log& log, std::string name)
        : m_log(log)
        , m_name(std::move(name))
    {
    }

    ~RecordingScene() override { record("destroy"); }

    void onLoad() override { record("load"); }
    void onEnter() override { record("enter"); }
    void onExit() override { record("exit"); }
    void onUpdate(float) override { record("update"); }
    void onImGui() override { record("imgui"); }

    void onEvent(Event& event) override
    {
        record("event");
        if (handlesEvents) {
            event.setHandled(true);
        }
    }

    bool handlesEvents = false;

protected:
    void record(const char* what) { m_log.push_back(m_name + "." + what); }

    Log& m_log;
    std::string m_name;
};

// Adds a child scene when it becomes active
class ParentScene : public RecordingScene {
public:
    using RecordingScene::RecordingScene;

    void onEnter() override
    {
        RecordingScene::onEnter();
        child = addChild<RecordingScene>(getLayer() + 1, m_log, m_name + "/child");
    }

    RecordingScene* child = nullptr;
};

class ViewScene : public Scene {
public:
    explicit ViewScene(std::vector<int> viewOrders)
        : m_viewOrders(std::move(viewOrders))
    {
    }

    void onLoad() override
    {
        for (int order : m_viewOrders) {
            createView()->setOrder(order);
        }
    }

private:
    std::vector<int> m_viewOrders;
};

struct LoadControl {
    std::atomic<bool> started{false};
    std::atomic<bool> release{false};
};

// Loads until the test releases it
class SlowLoadingScene : public Scene {
public:
    explicit SlowLoadingScene(LoadControl& control)
        : m_control(control)
    {
    }

    void onLoad() override
    {
        setLoadProgress(0.5f);
        m_control.started = true;
        while (!m_control.release.load()) {
            std::this_thread::yield();
        }
        loadThread = std::this_thread::get_id();
    }

    std::thread::id loadThread;

private:
    LoadControl& m_control;
};

bool waitFor(const std::atomic<bool>& flag)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!flag.load()) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        std::this_thread::yield();
    }
    return true;
}

std::vector<Scene*> scenesOf(const SceneManager& manager)
{
    std::vector<Scene*> scenes;
    for (const auto& scene : manager.getScenes()) {
        scenes.push_back(scene.get());
    }
    return scenes;
}

bool contains(const Log& log, const std::string& entry)
{
    return std::find(log.begin(), log.end(), entry) != log.end();
}

} // namespace

TEST(SceneManagerTest, ChangesAreAppliedAtTheNextUpdate)
{
    Log log;
    SceneManager manager(nullptr);

    Scene* scene = manager.add<RecordingScene>(0, log, "a");
    EXPECT_TRUE(manager.getScenes().empty());
    EXPECT_EQ(scene->getState(), Scene::State::Created);

    manager.update(0.0f);

    EXPECT_EQ(scenesOf(manager), std::vector<Scene*>{scene});
    EXPECT_TRUE(scene->isActive());
    EXPECT_EQ(log, (Log{"a.load", "a.enter", "a.update"}));
}

TEST(SceneManagerTest, ScenesAreSortedByLayer)
{
    Log log;
    SceneManager manager(nullptr);

    Scene* b = manager.add<RecordingScene>(100, log, "b");
    Scene* a = manager.add<RecordingScene>(0, log, "a");
    Scene* c = manager.add<RecordingScene>(100, log, "c");
    manager.update(0.0f);

    // Within a layer, the scene added later is above
    EXPECT_EQ(scenesOf(manager), (std::vector<Scene*>{a, b, c}));
}

TEST(SceneManagerTest, UpdatesFromTheBottomUp)
{
    Log log;
    SceneManager manager(nullptr);
    manager.add<RecordingScene>(100, log, "hud");
    manager.add<RecordingScene>(0, log, "main");
    manager.update(0.0f);

    log.clear();
    manager.update(0.0f);

    EXPECT_EQ(log, (Log{"main.update", "hud.update"}));
}

TEST(SceneManagerTest, BlocksUpdatePausesTheLayersBelow)
{
    Log log;
    SceneManager manager(nullptr);
    manager.add<RecordingScene>(0, log, "main");
    manager.add<RecordingScene>(100, log, "hud");
    Scene* pause = manager.add<RecordingScene>(200, log, "pause");
    pause->setBlocksUpdate(true);
    manager.update(0.0f);

    log.clear();
    manager.update(0.0f);
    EXPECT_EQ(log, (Log{"pause.update"}));

    pause->close();
    log.clear();
    manager.update(0.0f);
    EXPECT_EQ(log, (Log{"pause.exit", "pause.destroy", "main.update", "hud.update"}));
}

TEST(SceneManagerTest, ImGuiIsBuiltFromTheBottomUpEvenBelowABlockingScene)
{
    Log log;
    SceneManager manager(nullptr);
    manager.add<RecordingScene>(0, log, "main");
    Scene* pause = manager.add<RecordingScene>(200, log, "pause");
    pause->setBlocksUpdate(true);
    manager.update(0.0f);

    log.clear();
    manager.updateImGui();
    EXPECT_EQ(log, (Log{"main.imgui", "pause.imgui"}));
}

TEST(SceneManagerTest, EventsGoFromTheTopDown)
{
    Log log;
    SceneManager manager(nullptr);
    manager.add<RecordingScene>(0, log, "main");
    manager.add<RecordingScene>(100, log, "hud");
    manager.update(0.0f);

    log.clear();
    TestEvent event;
    manager.dispatchEvent(event);

    EXPECT_EQ(log, (Log{"hud.event", "main.event"}));
}

TEST(SceneManagerTest, HandledEventStopsAtTheScene)
{
    Log log;
    SceneManager manager(nullptr);
    manager.add<RecordingScene>(0, log, "main");
    manager.add<RecordingScene>(100, log, "hud")->handlesEvents = true;
    manager.update(0.0f);

    log.clear();
    TestEvent event;
    manager.dispatchEvent(event);

    EXPECT_EQ(log, (Log{"hud.event"}));
    EXPECT_TRUE(event.isHandled());
}

TEST(SceneManagerTest, BlocksInputStopsEventsFromReachingTheLayersBelow)
{
    Log log;
    SceneManager manager(nullptr);
    manager.add<RecordingScene>(0, log, "main");
    manager.add<RecordingScene>(100, log, "menu")->setBlocksInput(true);
    manager.update(0.0f);

    log.clear();
    TestEvent event;
    manager.dispatchEvent(event);

    EXPECT_EQ(log, (Log{"menu.event"}));
}

TEST(SceneManagerTest, ChangeRemovesScenesThatAreNotPersistent)
{
    Log log;
    SceneManager manager(nullptr);
    Scene* fade = manager.add<RecordingScene>(1000, log, "fade");
    fade->setPersistent(true);
    manager.change<RecordingScene>(log, "title");
    manager.update(0.0f);

    Scene* game = manager.change<RecordingScene>(log, "game");
    log.clear();
    manager.update(0.0f);

    EXPECT_EQ(scenesOf(manager), (std::vector<Scene*>{game, fade}));
    EXPECT_EQ(log, (Log{"title.exit", "title.destroy", "game.load", "game.enter", "game.update",
                        "fade.update"}));
}

TEST(SceneManagerTest, SceneRemovedBeforeItIsAddedIsNeverLoaded)
{
    Log log;
    SceneManager manager(nullptr);

    Scene* scene = manager.add<RecordingScene>(0, log, "a");
    manager.remove(scene);
    manager.update(0.0f);

    EXPECT_TRUE(manager.getScenes().empty());
    EXPECT_EQ(log, (Log{"a.destroy"}));
}

TEST(SceneManagerTest, ChangesRequestedFromCallbacksAreAppliedInTheSameUpdate)
{
    Log log;
    SceneManager manager(nullptr);

    auto* parent = manager.add<ParentScene>(0, log, "parent");
    manager.update(0.0f);

    ASSERT_NE(parent->child, nullptr);
    EXPECT_TRUE(parent->child->isActive());
    EXPECT_EQ(parent->child->getParent(), parent);
    EXPECT_EQ(scenesOf(manager), (std::vector<Scene*>{parent, parent->child}));
}

TEST(SceneManagerTest, ChildIsDestroyedBeforeItsParent)
{
    Log log;
    SceneManager manager(nullptr);
    auto* parent = manager.add<ParentScene>(0, log, "parent");
    manager.update(0.0f);

    parent->close();
    log.clear();
    manager.update(0.0f);

    EXPECT_TRUE(manager.getScenes().empty());
    EXPECT_EQ(log, (Log{"parent.exit", "parent/child.exit", "parent/child.destroy",
                        "parent.destroy"}));
}

TEST(SceneManagerTest, ChildOfARemovedParentIsNeverAdded)
{
    Log log;
    SceneManager manager(nullptr);
    Scene* parent = manager.add<RecordingScene>(0, log, "parent");
    manager.update(0.0f);

    parent->addChild<RecordingScene>(1, log, "child");
    parent->close();
    log.clear();
    manager.update(0.0f);

    EXPECT_TRUE(manager.getScenes().empty());
    EXPECT_FALSE(contains(log, "child.load"));
    EXPECT_TRUE(contains(log, "child.destroy"));
}

TEST(SceneManagerTest, DestroysScenesFromTheTopDown)
{
    Log log;
    {
        SceneManager manager(nullptr);
        manager.add<RecordingScene>(0, log, "main");
        manager.add<RecordingScene>(100, log, "hud");
        manager.update(0.0f);
        log.clear();
    }

    EXPECT_EQ(log, (Log{"hud.exit", "hud.destroy", "main.exit", "main.destroy"}));
}

TEST(SceneManagerTest, CollectsViewsByLayerThenOrder)
{
    SceneManager manager(nullptr);
    Scene* ui = manager.add<ViewScene>(100, std::vector<int>{0});
    Scene* main = manager.add<ViewScene>(0, std::vector<int>{1, -1});
    manager.update(0.0f);

    std::vector<View*> views;
    manager.collectViews(views);

    const auto& mainViews = main->getViews();
    const auto& uiViews = ui->getViews();
    ASSERT_EQ(views.size(), 3u);
    EXPECT_EQ(views[0], mainViews[1].get()); // order -1
    EXPECT_EQ(views[1], mainViews[0].get()); // order 1
    EXPECT_EQ(views[2], uiViews[0].get());
}

TEST(SceneManagerTest, LoadAsyncWithoutWorkersLoadsOnTheCallingThread)
{
    Log log;
    SceneManager manager(nullptr);
    manager.change<RecordingScene>(log, "title");
    manager.update(0.0f);

    SceneLoadOperation operation =
        manager.loadAsync<RecordingScene>(SceneManager::LoadMode::Single, 0, log, "game");
    EXPECT_TRUE(operation.isLoaded());
    EXPECT_FALSE(operation.isDone());

    manager.update(0.0f);

    EXPECT_TRUE(operation.isDone());
    ASSERT_EQ(manager.getScenes().size(), 1u);
    EXPECT_TRUE(contains(log, "title.destroy"));
    EXPECT_TRUE(contains(log, "game.enter"));
}

class SceneManagerAsyncTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        job::JobSystemConfig config;
        config.numWorkers = 2;
        job::JobSystem::getInstance().initialize(config);
    }

    void TearDown() override { job::JobSystem::getInstance().shutdown(); }

    // Run updates until the operation is loaded, or give up after a while
    static bool updateUntilLoaded(SceneManager& manager, const SceneLoadOperation& operation)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!operation.isLoaded()) {
            if (std::chrono::steady_clock::now() > deadline) {
                return false;
            }
            std::this_thread::yield();
        }
        manager.update(0.0f);
        return true;
    }
};

TEST_F(SceneManagerAsyncTest, LoadsOnAWorkerThreadAndActivatesOnTheMainThread)
{
    Log log;
    SceneManager manager(nullptr);
    Scene* loading = manager.change<RecordingScene>(log, "loading");
    manager.update(0.0f);

    LoadControl control;
    SceneLoadOperation operation =
        manager.loadAsync<SlowLoadingScene>(SceneManager::LoadMode::Single, 0, control);
    ASSERT_TRUE(waitFor(control.started));

    // The current scenes keep running while the new one loads
    manager.update(0.0f);
    EXPECT_FALSE(operation.isLoaded());
    EXPECT_FLOAT_EQ(operation.progress(), 0.5f);
    EXPECT_EQ(scenesOf(manager), std::vector<Scene*>{loading});

    control.release = true;
    ASSERT_TRUE(updateUntilLoaded(manager, operation));

    EXPECT_TRUE(operation.isDone());
    EXPECT_FLOAT_EQ(operation.progress(), 1.0f);
    ASSERT_EQ(manager.getScenes().size(), 1u);
    auto* game = static_cast<SlowLoadingScene*>(manager.getScenes().front().get());
    EXPECT_TRUE(game->isActive());
    EXPECT_NE(game->loadThread, std::this_thread::get_id());
    EXPECT_TRUE(contains(log, "loading.destroy"));
}

TEST_F(SceneManagerAsyncTest, WaitsForActivationWhenNotAllowed)
{
    Log log;
    SceneManager manager(nullptr);
    manager.change<RecordingScene>(log, "loading");
    manager.update(0.0f);

    SceneLoadOperation operation =
        manager.loadAsync<RecordingScene>(SceneManager::LoadMode::Additive, 10, log, "game");
    operation.setAllowActivation(false);

    ASSERT_TRUE(updateUntilLoaded(manager, operation));
    EXPECT_FALSE(operation.isDone());
    EXPECT_EQ(manager.getScenes().size(), 1u);

    operation.activate();
    manager.update(0.0f);

    EXPECT_TRUE(operation.isDone());
    // Additive keeps the loading scene
    EXPECT_EQ(manager.getScenes().size(), 2u);
}

TEST_F(SceneManagerAsyncTest, DestructionWaitsForAPendingLoad)
{
    LoadControl control;
    std::thread releaser;
    {
        SceneManager manager(nullptr);
        manager.loadAsync<SlowLoadingScene>(SceneManager::LoadMode::Single, 0, control);

        releaser = std::thread([&control] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            control.release = true;
        });
    }

    // The manager could only be destroyed once the load had finished
    EXPECT_TRUE(control.release.load());
    releaser.join();
}
