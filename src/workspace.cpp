#include <utility>
// TODO: remove later
#include <iostream>

#include "compilation_database.h"
#include "logger.h"
#include "utils.h"
#include "workspace.h"

bool operator&(const workspace_events &lhs, const workspace_events &rhs) {
    return (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)) != 0;
}

workspace_event::workspace_event(workspace_events event_mask, const fs::path &path)
    : m_event_mask(event_mask), m_affected_path(path) {}

workspace_events workspace_event::event_mask() const { return m_event_mask; }

const fs::path &workspace_event::affected_path() const { return m_affected_path; }

std::optional<int> workspace_event::directory_watch() const { return m_directory_watch; }

workspace_event &workspace_event::directory_watch(int watch) {
    m_directory_watch = watch;
    return *this;
}

workspace::workspace(int notify_fd, std::map<int, fs::path> directory_watches, const config &conf)
    : m_config(conf), m_notify_fd(notify_fd), m_directory_watches(directory_watches) {
    populate_relevant_files();
    update_compilation_database();
    m_event_handlers.emplace_back(default_handler);
}

workspace::workspace(workspace &&other)
    : m_config(std::move(other.m_config)),
      m_event_handlers(std::move(other.m_event_handlers)),
      m_notify_fd(other.m_notify_fd),
      m_directory_watches(std::move(other.m_directory_watches)),
      m_relevant_files(std::move(other.m_relevant_files)) {
    other.m_notify_fd = -1;
}

workspace::~workspace() {
    for (const auto &[directory_watch, path] : m_directory_watches) {
        inotify_rm_watch(m_notify_fd, directory_watch);
    }

    if (m_notify_fd != -1) {
        close(m_notify_fd);
    }
}

void workspace::swap(workspace &other) {
    using std::swap;

    swap(m_config, other.m_config);
    swap(m_event_handlers, other.m_event_handlers);
    swap(m_directory_watches, other.m_directory_watches);
    swap(m_relevant_files, other.m_relevant_files);
    swap(m_notify_fd, other.m_notify_fd);
    swap(m_compilation_database, other.m_compilation_database);
}

bool workspace::check_for_updates() {
    fd_set descriptor_set;
    FD_ZERO(&descriptor_set);
    FD_SET(m_notify_fd, &descriptor_set);

    int ret = pselect(m_notify_fd + 1, &descriptor_set, nullptr, nullptr, nullptr, nullptr);

    if (ret == -1) {
        perror("Error selecting");
        return false;
    }

    if (FD_ISSET(m_notify_fd, &descriptor_set)) {
        inotify_handler();
    }

    update_compilation_database();
    return true;
}

void workspace::default_handler(workspace &workspace_instance, const workspace_event &event) {
    if (!workspace_instance.is_relevant_file(event.affected_path())) {
        return;
    }

    if (auto directory_watch = event.directory_watch();
        directory_watch.has_value() && event.event_mask() & workspace_events::deleted_self) {
        auto result = workspace_instance.m_directory_watches.find(*directory_watch);

        if (result == workspace_instance.m_directory_watches.cend()) {
            return;
        }

        inotify_rm_watch(workspace_instance.m_notify_fd, result->first);
        workspace_instance.m_directory_watches.erase(result);
        std::cout << "Removed watch for " << event.affected_path().c_str() << std::endl;
    }

    // TODO add whitelist/blacklist to this maybe
    if (fs::is_directory(event.affected_path())) {
        if (event.event_mask() & workspace_events::created) {
            int directory_watch =
                inotify_add_watch(workspace_instance.m_notify_fd, event.affected_path().c_str(), IN_ALL_EVENTS);

            if (directory_watch == -1) {
                return;
            }

            workspace_instance.m_directory_watches.emplace(directory_watch, event.affected_path());
            std::cout << "Added directory watch for " << event.affected_path().c_str() << std::endl;
        }
    } else {
        // TODO only watch for source or header files
        if (event.event_mask() & workspace_events::created) {
            std::cout << "Added file " << event.affected_path().c_str() << std::endl;
            auto updated = workspace_instance.m_relevant_files.emplace(event.affected_path());

            if (updated.second) {
                workspace_instance.compilation_database_is_dirty();
            }
        }

        if (event.event_mask() & workspace_events::moved_to) {
            std::cout << "Moved file to " << event.affected_path().c_str() << std::endl;
            auto updated = workspace_instance.m_relevant_files.emplace(event.affected_path());

            if (updated.second) {
                workspace_instance.compilation_database_is_dirty();
            }
        }

        if (event.event_mask() & workspace_events::modified) {
            std::cout << "File was modified " << event.affected_path().c_str() << std::endl;
        }

        if (event.event_mask() & workspace_events::moved_from || event.event_mask() & workspace_events::deleted) {
            std::cout << "Deleted/Moved file from " << event.affected_path().c_str() << std::endl;

            auto to_remove = std::find(workspace_instance.m_relevant_files.cbegin(),
                                       workspace_instance.m_relevant_files.cend(), event.affected_path());

            if (to_remove != workspace_instance.m_relevant_files.cend()) {
                workspace_instance.m_relevant_files.erase(to_remove);
                workspace_instance.compilation_database_is_dirty();
            }
        }

        if (event.event_mask() & workspace_events::closed_write) {
            std::cout << "Close write file " << event.affected_path().c_str() << std::endl;
            // Add flags back to compile_commands.json
        }
    }

    // std::cout << "Relevant files : " << '\n';
    // for (const auto &current_element : m_relevant_files) {
    //     std::cout << current_element << '\n';
    // }
    // std::cout << std::endl;
}

void workspace::inotify_handler() {
    constexpr size_t buffer_size = 4096;
    std::aligned_storage_t<buffer_size, sizeof(inotify_event)> aligned_buffer;

    char *as_char_buffer = reinterpret_cast<char *>(&aligned_buffer);
    const inotify_event *event = nullptr;
    ssize_t len_read = 0;

    while (1) {
        len_read = read(m_notify_fd, as_char_buffer, buffer_size);

        if (len_read == -1 && errno != EAGAIN) {
            return;
        }

        if (len_read <= 0) {
            return;
        }

        for (auto it = as_char_buffer; it < as_char_buffer + len_read; it += sizeof(inotify_event) + event->len) {
            event = reinterpret_cast<const inotify_event *>(it);
            auto result = m_directory_watches.find(event->wd);

            if (result == m_directory_watches.cend()) {
                // TODO print warning that no matching inotify watch was found
                continue;
            }

            auto affected_path = result->second / fs::path(event->name);
            bool is_directory = fs::is_directory(affected_path);

            send_event(event->mask, affected_path, event->wd);
        }
    }
}

void workspace::send_event(uint32_t mask, const fs::path &affected_path, int directory_watch) {
    workspace_event event(workspace_events(mask), affected_path);
    event.directory_watch(directory_watch);

    for (const auto &current_function : m_event_handlers) {
        if (current_function) {
            current_function(*this, event);
        }
    }
}

bool workspace::is_relevant_file(const fs::path &path, const config &conf) {
    const std::vector<std::regex> &prepared_blacklist_patterns = conf.prepared_blacklist_patterns();
    const std::vector<std::regex> &prepared_whitelist_patterns = conf.prepared_whitelist_patterns();
    const auto path_as_string = path.string();

    const auto match_func = [&path_as_string](auto &current_pattern) {
        bool result = std::regex_search(path_as_string.cbegin(), path_as_string.cend(), current_pattern);
        return result;
    };

    if (fs::is_regular_file(path) && !path.has_extension()) {
        return false;
    }

    if (fs::is_regular_file(path) && path.has_extension() && path.extension().string().back() == '~') {
        std::cout << path << std::endl;
        return false;
    }

    // TODO: add back blacklist functionality
    // bool relevant = std::none_of(prepared_blacklist_patterns.cbegin(), prepared_blacklist_patterns.cend(),
    // match_func);
    bool relevant = std::any_of(prepared_whitelist_patterns.cbegin(), prepared_whitelist_patterns.cend(), match_func);

    return relevant;
}

bool workspace::is_relevant_file(const fs::path &path) const { return is_relevant_file(path, m_config); }

const fs::path workspace::project_root() const { return *m_config.project_root(); }

void workspace::populate_relevant_files() {
    for (auto &current_entry : fs::recursive_directory_iterator(*m_config.project_root())) {
        if (is_relevant_file(current_entry)) {
            m_relevant_files.emplace(current_entry.path());
        }
    }
}

void workspace::compilation_database_is_dirty() { m_dirty_compilation_database = true; }

void workspace::update_compilation_database() {
    auto logger_instance = logger::instance();
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!m_dirty_compilation_database) {
        return;
    }

    if (!m_config.compilation_database_path()) {
        logger_instance->log_error("Can't open compilation database, there's no valid path");
        return;
    }

    m_compilation_database = compilation_database::read_from(*m_config.compilation_database_path());

    if (!m_compilation_database) {
        logger_instance->log_error("Couldn't read database");
        return;
    }

    bool added_files = m_compilation_database->add_missing_files(m_relevant_files, m_config);

    if (!added_files) {
        logger_instance->log_error("Compilation database is already up to date");
        return;
    }

    auto tmp_file = project_root() / "comp_db.json";
    m_compilation_database->write_to(tmp_file);
    std::error_code ec;
    fs::rename(tmp_file, project_root() / compilation_database::database_name, ec);
    m_dirty_compilation_database = false;
}

std::optional<workspace> workspace::discover_project(const fs::path &project_path, const std::optional<config> &conf) {
    if (!fs::exists(project_path) || !fs::is_directory(project_path)) {
        return std::nullopt;
    }

    std::optional<config> created_config{};
    if (conf) {
        created_config = conf.value();
    }

    if (!created_config) {
        created_config = config::load_config(project_path / "damnflags_conf.json");

        if (!created_config) {
            created_config = config::create_empty_config();
        }
    }

    if (!created_config) {
        // Shouldn't happen
        return std::nullopt;
    }

    config resulting_config = created_config.value();
    resulting_config.project_root(project_path);

    int notify_fd = inotify_init1(IN_NONBLOCK);
    std::map<int, fs::path> directory_watches;

    if (notify_fd == -1) {
        return std::nullopt;
    }

    for (auto &current_entry : fs::recursive_directory_iterator(project_path)) {
        if (current_entry.is_directory() && is_relevant_file(current_entry, resulting_config)) {
            auto absolute_path = fs::absolute(current_entry);
            int watch_directory = inotify_add_watch(notify_fd, absolute_path.c_str(), IN_ALL_EVENTS);

            if (watch_directory != -1) {
                std::cout << "Adding " << absolute_path.c_str() << " to the list of watched directories" << std::endl;
                directory_watches.emplace(watch_directory, absolute_path);
            }
        } else if (current_entry.is_regular_file() && is_relevant_file(current_entry, resulting_config)) {
            if (current_entry.path().filename() == compilation_database::database_name) {
                resulting_config.compilation_database_path(fs::absolute(current_entry.path()));
                auto absolute_path = fs::absolute(current_entry);
                int watch_compilation_database = inotify_add_watch(notify_fd, absolute_path.c_str(), IN_ALL_EVENTS);

                if (watch_compilation_database != -1) {
                    std::cout << "Adding compilation_database : " << absolute_path.c_str()
                              << " to the list of watched things" << std::endl;
                    directory_watches.emplace(watch_compilation_database, absolute_path);
                }
            }
        }
    }

    return workspace(std::move(notify_fd), std::move(directory_watches), resulting_config);
}
