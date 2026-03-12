#include <ocf/Application.h>
#include <ocf/Scene.h>
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace ocf;

void setup(Engine* engine, Scene* scene)
{
}

void cleanup(Engine* engine, Scene* scene)
{
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Application& app = Application::getInstance();
    Config config;
    config.title = "title";

    app.run(config, setup, cleanup);

    return 0;
}
