#include <ocf/platform/Application.h>
#include <ocf/resource/ModelImporter.h>
#include <ocf/scene/MeshInstance.h>
#include <ocf/scene/Scene.h>
#include <ocf/scene/Node.h>
#include <ocf/resource/Mesh.h>
#include <ocf/scene/View.h>
#include <ocf/scene/Camera.h>
#include <ocf/math/geometric.h>
#include <ocf/platform/FileSystem.h>
#include <ocf/renderer/Material.h>
#include <ocf/renderer/MaterialInstance.h>
#include <ocf/core/Engine.h>
#include <ocf/rhi/Handle.h>
#include <ocf/rhi/PipelineState.h>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif


using namespace ocf;
using namespace ocf::rhi;

Ref<Mesh> g_mesh;
Node* g_node = nullptr;

void update(float deltaTime)
{
    if (g_node) {
        g_node->setRotation(g_node->getRotation() + math::vec3(0.0f, 16.0f * deltaTime, 0.0f));
    }
}

void setup(Engine& engine, View* view, Scene *scene)
{
    scene->setUpdateCallback(update);

    auto cameraNode = scene->getRoot()->createChild();
    auto camera = cameraNode->addComponent<Camera>();
    camera->perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    camera->lookAt(math::vec3(0.0f, 6.0f, 10.0f), math::vec3(0.0f, 0.0f, 0.0f),
                   math::vec3(0.0f, 1.0f, 0.0f));
    view->setCamera(camera);

    auto vsPath = FileSystem::getInstance()->getAssetFullPath("shaders/basic_pbr.vert.spv");
    auto fsPath = FileSystem::getInstance()->getAssetFullPath("shaders/basic_pbr.frag.spv");

    ModelImporter importer;
    auto result = importer.loadFromFile("models/AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf");
    if (result.isOk()) {
        g_mesh = result.unwrap();
        g_mesh->createSubMeshBuffers(engine);

        g_node = scene->getRoot()->createChild();

        auto meshInstance = g_node->addComponent<MeshInstance>(engine, vsPath, fsPath);
        meshInstance->setMesh(g_mesh);
    }
}

void cleanup(Engine& engine, View* view, Scene *scene)
{
    if (g_mesh.ptr() != nullptr) {
        g_mesh->terminate(engine);
    }
}

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
