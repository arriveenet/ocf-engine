// SPDX-License-Identifier: MIT

#include "ocf/core/Logger.h"
#include <format>

namespace ocf::logger {

PatternLayout::PatternLayout(const std::string& pattern)
    : m_pattern(pattern)
{
}

std::string PatternLayout::format(const LogEvent& event)
{
    return std::format("[{}][{}] {}", event.getTimestamp(), toString(event.getLevel()),
                       event.getMessage());
}


Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

void Logger::addAppender(std::unique_ptr<Appender> appender)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_appenders.emplace_back(std::move(appender));
}

} // namespace ocf::logger
