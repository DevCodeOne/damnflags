#include "logger.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

logger_configuration &logger_configuration::should_log_to_console(bool value) {
    m_should_log_to_console = value;

    return *this;
}

logger_configuration &logger_configuration::log_path(const fs::path &value) {
    m_log_path = value;

    return *this;
}

bool logger_configuration::should_log_to_console() const { return m_should_log_to_console; }

const fs::path &logger_configuration::log_path() const { return m_log_path; }

std::shared_ptr<logger> logger::instance(const std::optional<logger_configuration> config) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance && config) {
        _instance = std::shared_ptr<logger>(new logger(*config));
    }

    return _instance;
}

logger::logger(const logger_configuration &config)
    : m_logger_instance(config.should_log_to_console() ? spdlog::stdout_color_mt("default")
                                                       : spdlog::basic_logger_mt("default", "log.txt")) {
    // TODO: set debugging level
    m_logger_instance->set_level(spdlog::level::debug);
}

// TODO: maybe allow fmt type input directly and pass it on to the spdlog lib
void logger::log_info(const std::string &info) {
    if (!m_logger_instance) {
        return;
    }

    m_logger_instance->info(info);
}

void logger::log_warning(const std::string &warning) {
    if (!m_logger_instance) {
        return;
    }

    m_logger_instance->warn(warning);
}

void logger::log_error(const std::string &error) {
    if (!m_logger_instance) {
        return;
    }

    m_logger_instance->error(error);
}
