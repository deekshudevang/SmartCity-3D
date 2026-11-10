#pragma once

#include <memory>
#include <string>

namespace SmartCity::core {

struct AppCommandLineArgs {
    int count = 0;
    char** args = nullptr;
    bool headless = false;
    bool screenshots = false;
    std::string configPath = "assets/config/simulation_config.json";
    std::string intersectionPath = "assets/config/intersection_config.json";
};

class Application {
public:
    Application(const AppCommandLineArgs& args);
    ~Application();

    int Run();

private:
    void Init();
    void Shutdown();

    AppCommandLineArgs m_args;
    bool m_running;
};

} // namespace SmartCity::core
