#include <ocf/platform/Application.h>
#include <ocf/resource/ModelImporter.h>
#include <ocf/scene/MeshInstance.h>
#include <ocf/scene/Scene.h>
#include <ocf/scene/Node.h>
#include <ocf/resource/Mesh.h>
#include <ocf/scene/View.h>
#include <ocf/scene/Camera.h>
#include <ocf/math/geometric.h>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif


using namespace ocf;

void setup(Engine& engine, View* view, Scene *scene) {
    auto cameraNode = scene->getRoot()->createChild();
    auto camera = cameraNode->addComponent<Camera>();
    camera->perspective(math::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    camera->lookAt(math::vec3(0.0f, 0.0f, 5.0f), math::vec3(0.0f, 0.0f, 0.0f),
                   math::vec3(0.0f, 1.0f, 0.0f));
    view->setCamera(camera);

    Mesh* mesh = new Mesh();
    ModelImporter importer;
    if(importer.import("models/utah_teapot.obj", *mesh)) {
        auto node = scene->getRoot()->createChild();
        auto meshInstance = node->addComponent<MeshInstance>();
        meshInstance->setMesh(mesh);
    }
    else {
        delete mesh;
    }
}

void cleanup(Engine& engine, View* view, Scene *scene) {}

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
