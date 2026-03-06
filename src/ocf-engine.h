#pragma once

namespace ocf {

class Engine {
public:
    Engine();
    ~Engine();

    bool init();
    void run();
    void quit();

private:
    bool m_running;
};

} // namespace ocf
