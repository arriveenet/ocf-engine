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

Mesh* g_mesh = nullptr;

void setup(Engine& engine, View* view, Scene *scene)
{
    auto cameraNode = scene->getRoot()->createChild();
    auto camera = cameraNode->addComponent<Camera>();
    camera->perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    camera->lookAt(math::vec3(5.0f, 5.0f, 5.0f), math::vec3(0.0f, 0.0f, 0.0f),
                   math::vec3(0.0f, 1.0f, 0.0f));
    view->setCamera(camera);

    auto vsPath = FileSystem::getInstance()->getAssetFullPath("shaders/basic_pbr.vert.spv");
    auto fsPath = FileSystem::getInstance()->getAssetFullPath("shaders/basic_pbr.frag.spv");

    g_mesh = new Mesh();
    ModelImporter importer;
    if(importer.import("models/BoxTextured/glTF/BoxTextured.gltf", *g_mesh)) {
        g_mesh->createSubMeshBuffers(engine);
        auto node = scene->getRoot()->createChild();
        auto meshInstance = node->addComponent<MeshInstance>(engine, vsPath, fsPath);
        meshInstance->setMesh(g_mesh);
    }
    else {
        delete g_mesh;
        g_mesh = nullptr;
    }
}

void cleanup(Engine& engine, View* view, Scene *scene)
{
    if (g_mesh) {
        g_mesh->terminate(engine);
        delete g_mesh;
        g_mesh = nullptr;
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
