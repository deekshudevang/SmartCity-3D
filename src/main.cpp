#include "core/Application.h"
#include <string>

int main(int argc, char* argv[]) {
    SmartCity::core::AppCommandLineArgs args;
    args.count = argc;
    args.args = argv;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            args.headless = true;
        } else if (arg == "--screenshots" || arg == "--phase6-screenshots") {
            args.screenshots = true;
        } else if (arg == "--phase8-screenshots" || arg == "--release-screenshots") {
            args.releaseScreenshots = true;
        } else if (arg == "--benchmark") {
            args.benchmark = true;
        } else if (arg == "--benchmark-duration" && i + 1 < argc) {
            args.benchmarkDuration = std::stof(argv[++i]);
        } else if (arg == "--benchmark-vehicles" && i + 1 < argc) {
            args.benchmarkVehicles = std::stoi(argv[++i]);
        } else if (arg == "--no-shadows") {
            args.noShadows = true;
        } else if (arg == "--no-rain") {
            args.noRain = true;
        } else if (arg == "--demo") {
            args.demoMode = true;
        } else if (arg == "--config" && i + 1 < argc) {
            args.configPath = argv[++i];
        } else if (arg == "--intersection" && i + 1 < argc) {
            args.intersectionPath = argv[++i];
        }
    }

    SmartCity::core::Application app(args);
    return app.Run();
}
