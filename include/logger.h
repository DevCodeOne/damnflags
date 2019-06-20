#pragma once

#include <memory>
#include <mutex>
#include <optional>

#include "filesys.h"

class logger_configuration final {
   public:
    logger_configuration() = default;

    logger_configuration &should_log_to_console(bool value);
    logger_configuration &log_path(const fs::path &value);

    bool should_log_to_console() const;
    const fs::path &log_path() const;

   private:
    bool m_should_log_to_console = false;
    fs::path m_log_path;
};

class logger final {
   public:
    static std::shared_ptr<logger> instance(std::optional<logger_configuration> config = {});

   private:
    logger(const logger_configuration &configuration);

    static inline std::shared_ptr<logger> _instance;
    static inline std::mutex _instance_mutex;
};
