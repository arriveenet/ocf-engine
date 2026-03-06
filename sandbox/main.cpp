#include <ocf-engine.h>

int main(int argc, char* argv[])
{
    ocf::Engine engine;

    if (!engine.init()) {
        return 1;
    }

    engine.run();
    engine.quit();

    return 0;
}
