#pragma once

#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>

#include "filesys.h"

class project_structure final {
   public:
    static std::optional<project_structure> discover_project(const fs::path &project_path);
    project_structure(const project_structure &other) = delete;
    project_structure(project_structure &&other);
    ~project_structure();

    project_structure &operator=(const project_structure &other) = delete;
    project_structure &operator=(project_structure &&other) = default;

    void swap(project_structure &other);

    bool check_for_updates();

    const fs::path &project_root() const;
    const fs::path &compilation_database() const;

   private:
    project_structure(fs::path project_root, fs::path compilation_database, int notify_fd,
                      std::map<int, fs::path> directory_watches);

    void populate_relevant_files();
    void inotify_handler();
    void send_event(uint32_t mask, const fs::path &affected_path, int directory_watch);
    void default_handler(uint32_t mask, const fs::path &affected_path, int directory_watch);
    bool is_relevant_file(const fs::path &file);

    fs::path m_project_root;
    fs::path m_compilation_database;
    std::map<int, fs::path> m_directory_watches;
    // Replace with std::set
    std::set<fs::path> m_relevant_files;
    int m_notify_fd = 0;
};
