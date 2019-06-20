#include <chrono>
#include <iostream>

#include "clara.hpp"

#include "config.h"
#include "logger.h"
#include "workspace.h"

int main(int argc, char *argv[]) {
    bool log_to_console = false;
    bool generate_config = false;
    std::string config_path = "";
    std::string log_path = "";
    std::string project_root = "";

    auto cli = clara::Opt(log_to_console, "log_to_console")["--log_to_console"]("Should log to console") |
               clara::Opt(config_path, "config_path")["--config"]("Path to the config") |
               clara::Opt(generate_config, "generate_config")["--generate_config"](
                   "Specify to print a config with all possible values to stdout") |
               clara::Opt(project_root, "project_root")["--project-root"]("Specify the project root");

    auto result = cli.parse(clara::Args(argc, argv));

    if (!result) {
        std::cerr << "Couldn't parse commandline: " << result.errorMessage() << std::endl;
        return EXIT_FAILURE;
    }

    if (generate_config) {
        std::cout << config::default_config() << std::endl;
    }

    logger_configuration log_config;
    log_config.should_log_to_console(log_to_console).log_path(log_path);

    auto logger = logger::instance(log_config);

    auto config = config::load_config(config_path);

    auto config_project_root = config->project_root();
    auto structure = workspace::discover_project(
        config_project_root ? *config_project_root : std::filesystem::current_path(), config);

    if (!structure) {
        return EXIT_FAILURE;
    }

    std::cout << "Starting damnflags in " << structure->project_root() << std::endl;

    while (1) {
        auto then = std::chrono::steady_clock::now();

        structure->check_for_updates();

        std::this_thread::sleep_until(then + std::chrono::milliseconds{500});
    }

    return EXIT_SUCCESS;
}
