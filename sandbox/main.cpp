#include <ocf/Engine.h>
#include <ocf/Scene.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace ocf;

int main(int argc, char* argv[])
{
#ifdef _WIN32
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Scene scene;

    Engine engine;

    if (!engine.init("Title", 800, 600, &scene)) {
        return 1;
    }

    engine.run();
    engine.terminate();

    return 0;
}
