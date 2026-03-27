#include <ocf/platform/Application.h>

using namespace ocf;

void setup(Engine& engine, Scene *scene) {}

void cleanup(Engine& engine, Scene *scene) {}

int main()
{
    Application::Config config;
    config.title = "SandBox Test";

    Application& app = Application::getInstance();
    app.run(config, setup, cleanup);
}
