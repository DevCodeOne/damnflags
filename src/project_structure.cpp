#include <iostream>
#include <utility>

#include "compilation_database.h"
#include "project_structure.h"
#include "utils.h"

project_structure::project_structure(fs::path project_root, fs::path compilation_database, int notify_fd,
                                     std::map<int, fs::path> directory_watches)
    : m_project_root(project_root),
      m_compilation_database_path(compilation_database),
      m_notify_fd(notify_fd),
      m_directory_watches(directory_watches) {
    populate_relevant_files();
    update_compilation_database();
}

project_structure::project_structure(project_structure &&other)
    : m_project_root(std::move(other.m_project_root)),
      m_compilation_database_path(std::move(other.m_compilation_database_path)),
      m_notify_fd(other.m_notify_fd),
      m_directory_watches(std::move(other.m_directory_watches)),
      m_relevant_files(std::move(other.m_relevant_files)) {
    other.m_notify_fd = -1;
}

project_structure::~project_structure() {
    for (const auto &[directory_watch, path] : m_directory_watches) {
        inotify_rm_watch(m_notify_fd, directory_watch);
    }

    if (m_notify_fd != -1) {
        close(m_notify_fd);
    }
}

void project_structure::swap(project_structure &other) {
    using std::swap;

    swap(m_compilation_database_path, other.m_compilation_database_path);
    swap(m_project_root, other.m_project_root);
    swap(m_directory_watches, other.m_directory_watches);
    swap(m_notify_fd, other.m_notify_fd);
}

bool project_structure::check_for_updates() {
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
        return true;
    }

    return false;
}

void project_structure::default_handler(uint32_t mask, const fs::path &affected_path, int directory_watch) {
    if (mask & IN_DELETE_SELF) {
        auto result = m_directory_watches.find(directory_watch);

        if (result == m_directory_watches.cend()) {
            return;
        }

        inotify_rm_watch(m_notify_fd, result->first);
        m_directory_watches.erase(result);
        std::cout << "Removed watch for " << affected_path.c_str() << std::endl;
    }

    if (fs::is_directory(affected_path)) {
        if (mask & IN_CREATE) {
            int directory_watch = inotify_add_watch(m_notify_fd, affected_path.c_str(), IN_ALL_EVENTS);

            if (directory_watch == -1) {
                return;
            }

            m_directory_watches.emplace(directory_watch, affected_path);
            std::cout << "Added directory watch for " << affected_path.c_str() << std::endl;
        }
    } else {
        if (!is_relevant_file(affected_path)) {
            return;
        }

        if (mask & IN_CREATE) {
            std::cout << "Added file " << affected_path.c_str() << std::endl;
            m_relevant_files.emplace(affected_path);
            update_compilation_database();
        }

        if (mask & IN_MOVED_TO) {
            std::cout << "Moved file to " << affected_path.c_str() << std::endl;
            m_relevant_files.emplace(affected_path);
            update_compilation_database();
        }

        if (mask & IN_MODIFY) {
            std::cout << "File was modified " << affected_path.c_str() << std::endl;
        }

        if (mask & IN_MOVED_FROM || mask & IN_DELETE) {
            std::cout << "Deleted/Moved file from " << affected_path.c_str() << std::endl;

            auto to_remove = std::find(m_relevant_files.cbegin(), m_relevant_files.cend(), affected_path);

            if (to_remove != m_relevant_files.cend()) {
                m_relevant_files.erase(to_remove);
            }

            update_compilation_database();
        }

        if (mask & IN_CLOSE_WRITE) {
            std::cout << "Close write file " << affected_path.c_str() << std::endl;
            // Add flags back to compile_commands.json
        }
    }

    // std::cout << "Relevant files : " << '\n';
    // for (const auto &current_element : m_relevant_files) {
    //     std::cout << current_element << '\n';
    // }
    // std::cout << std::endl;
}

void project_structure::inotify_handler() {
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

void project_structure::send_event(uint32_t mask, const fs::path &affected_path, int directory_watch) {
    default_handler(mask, affected_path, directory_watch);
    // for (const auto &[current_mask, current_function] : m_event_handler) {
    //     if (current_mask & mask) {
    //         current_function(mask, affected_path, directory_watch);
    //     }
    // }
}

bool project_structure::is_relevant_file(const fs::path &path) {
    if (!path.has_extension()) {
        return false;
    }

    if (is_source_file(path) || is_header_file(path)) {
        return true;
    }

    return path.filename() == compilation_database::database_name;
}

const fs::path &project_structure::project_root() const { return m_project_root; }

void project_structure::populate_relevant_files() {
    for (auto &current_entry : fs::recursive_directory_iterator(m_project_root)) {
        if (is_relevant_file(current_entry)) {
            m_relevant_files.emplace(current_entry.path());
        }
    }
}

void project_structure::update_compilation_database() {
    m_compilation_database = compilation_database::read_from(m_compilation_database_path);

    if (m_compilation_database) {
        bool added_files = m_compilation_database->add_missing_files(m_relevant_files);

        if (added_files) {
            auto tmp_file = project_root() / "comp_db.json";
            m_compilation_database->write_to(tmp_file);
            std::error_code ec;
            fs::rename(tmp_file, project_root() / compilation_database::database_name, ec);
        }
    }
}

std::optional<project_structure> project_structure::discover_project(const fs::path &project_path) {
    if (!fs::exists(project_path) || !fs::is_directory(project_path)) {
        return std::nullopt;
    }

    int notify_fd = inotify_init1(IN_NONBLOCK);
    std::map<int, fs::path> directory_watches;
    fs::path compilation_database;

    if (notify_fd == -1) {
        return std::nullopt;
    }

    for (auto &current_entry : fs::recursive_directory_iterator(project_path)) {
        if (current_entry.is_directory()) {
            auto absolute_path = fs::absolute(current_entry);
            int watch_directory = inotify_add_watch(notify_fd, absolute_path.c_str(), IN_ALL_EVENTS);

            if (watch_directory != -1) {
                std::cout << "Adding " << absolute_path.c_str() << " to the list of watched directories" << std::endl;
                directory_watches.emplace(watch_directory, absolute_path);
            }
        } else if (current_entry.is_regular_file() &&
                   current_entry.path().filename() == compilation_database::database_name) {
            compilation_database = fs::absolute(current_entry.path());
        }
    }

    return project_structure(std::move(project_path), std::move(compilation_database), std::move(notify_fd),
                             std::move(directory_watches));
}
