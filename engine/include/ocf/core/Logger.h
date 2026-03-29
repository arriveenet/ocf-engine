/* SPDX - License - Identifier : MIT */

#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace ocf {
namespace logger {

enum class LogLevel {
    Trace   = 0,
    Debug   = 1,
    Info    = 2,
    Warn    = 3,
    Error   = 4,
    Fatal   = 5
};

class LogEvent {
public:
    LogEvent(LogLevel level, const std::string& message)
        : m_level(level)
        , m_message(message)
        , m_timestamp(std::chrono::system_clock::now())
    {
    }

    LogLevel getLevel() const { return m_level; }

    const std::string& getMessage() const { return m_message; }

    std::chrono::system_clock::time_point getTimestamp() const { return m_timestamp; }

private:
    LogLevel m_level;
    std::string m_message;
    std::chrono::system_clock::time_point m_timestamp;
};

constexpr const char* toString(LogLevel level)
{
    switch (level) {
    case LogLevel::Trace:   return "TRACE";
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warn:    return "WARN";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Fatal:   return "FATAL";
    default:                return "UNKNOWN";
    }
}

class Layout {
public:
    virtual ~Layout() = default;
    virtual std::string format(const LogEvent& event) = 0;
};

class PatternLayout : public Layout {
public:
    PatternLayout(const std::string& pattern);
    std::string format(const LogEvent& event) override;

private:
    std::string m_pattern;
};

class Appender {
public:
    virtual ~Appender() = default;

    virtual void append(const LogEvent& event) = 0;

    void setLayout(std::unique_ptr<Layout> layout)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_layout = std::move(layout);
    }

protected:
    std::unique_ptr<Layout>m_layout;
    std::mutex m_mutex;
};

class ConsoleAppender : public Appender {
public:
    void append(const LogEvent& event) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_layout) {
            std::cout << m_layout->format(event) << std::endl;
        } else {
            std::cout << "[" << toString(event.getLevel()) << "] " << event.getMessage() << std::endl;
        }
    }
};

class Logger {
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);

    void addAppender(std::unique_ptr<Appender> appender);

    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args)
    {
        if (level < m_level) {
            return;
        }

        std::string message = std::vformat(fmt, std::make_format_args(args...));
        LogEvent event(level, message);

        for (auto& appender : m_appenders) {
            appender->append(event);
        }
    }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_level = LogLevel::Info;
    std::vector<std::unique_ptr<Appender>> m_appenders;
    std::mutex m_mutex;
};

#define OCF_LOG_TRACE(...)                                                                         \
    ocf::logger::Logger::getInstance().log(ocf::logger::LogLevel::Trace, __VA_ARGS__)
#define OCF_LOG_DEBUG(...)                                                                         \
    ocf::logger::Logger::getInstance().log(ocf::logger::LogLevel::Debug, __VA_ARGS__)
#define OCF_LOG_INFO(...)                                                                          \
    ocf::logger::Logger::getInstance().log(ocf::logger::LogLevel::Info, __VA_ARGS__)
#define OCF_LOG_WARN(...)                                                                          \
    ocf::logger::Logger::getInstance().log(ocf::logger::LogLevel::Warn, __VA_ARGS__)
#define OCF_LOG_ERROR(...)                                                                         \
    ocf::logger::Logger::getInstance().log(ocf::logger::LogLevel::Error, __VA_ARGS__)
#define OCF_LOG_FATAL(...)                                                                         \
    ocf::logger::Logger::getInstance().log(ocf::logger::LogLevel::Fatal, __VA_ARGS__)

} // namespace logger
} // namespace ocf
