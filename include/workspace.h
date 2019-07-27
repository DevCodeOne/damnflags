#pragma once

// clang-format off
#define POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <cstdint>
// clang-format off

#include <bitset>
#include <functional>
#include <map>
#include <optional>
#include <set>

#include "compilation_database.h"
#include "config.h"
#include "filesys.h"

enum struct workspace_events : uint32_t {
    accessed = IN_ACCESS,
    attribute_changed = IN_ATTRIB,
    closed_write = IN_CLOSE_WRITE,
    closed_nowrite = IN_CLOSE_NOWRITE,
    created = IN_CREATE,
    deleted = IN_DELETE,
    deleted_self = IN_DELETE_SELF,
    modified = IN_MODIFY,
    moved_self = IN_MOVE_SELF,
    moved_from = IN_MOVED_FROM,
    moved_to = IN_MOVED_TO,
    opened = IN_OPEN
};

bool operator&(const workspace_events &lhs, const workspace_events &rhs);

class workspace_event final {
    public:
        workspace_event(workspace_events event_mask, const fs::path &path);

        workspace_events event_mask() const;
        const fs::path &affected_path() const;

    private:
        std::optional<int> directory_watch() const;

        workspace_event &directory_watch(int watch);

        const workspace_events m_event_mask;
        const fs::path m_affected_path;
        std::optional<int> m_directory_watch{};

        friend class workspace;
};

class workspace final {
   public:
    using handler_type = std::function<void(workspace &workspace_instance, const workspace_event &event)>;

    static std::optional<workspace> discover_project(const fs::path &project_path, const std::optional<config> &conf = {});
    static bool is_relevant_file(const fs::path &file, const config &conf);
    workspace(const workspace &other) = delete;
    workspace(workspace &&other);
    ~workspace();

    workspace &operator=(const workspace &other) = delete;
    workspace &operator=(workspace &&other) = default;

    void swap(workspace &other);
    void compilation_database_is_dirty();

    bool check_for_updates();

    const fs::path project_root() const;
    const fs::path compilation_database_path() const;

   private:
    workspace(int notify_fd,
              std::map<int, fs::path> directory_watches, const config &conf);

    void update_compilation_database();
    void populate_relevant_files();
    void inotify_handler();
    void send_event(uint32_t mask, const fs::path &affected_path, int directory_watch);
    static void default_handler(workspace &workspace_instance, const workspace_event &event);
    bool is_relevant_file(const fs::path &file) const;

    config m_config;
    std::vector<handler_type> m_event_handlers;
    std::map<int, fs::path> m_directory_watches;
    std::set<fs::path> m_relevant_files;
    int m_notify_fd = 0;
    bool m_dirty_compilation_database = true;
    std::optional<compilation_database> m_compilation_database;
};
