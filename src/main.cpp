#include <iostream>
#include <chrono>

#include "project_structure.h"

int main() {
    auto structure = project_structure::discover_project(std::filesystem::current_path());

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
