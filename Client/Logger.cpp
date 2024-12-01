#include "pch.h"
#include "Logger.h"

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::AddOutput(std::unique_ptr<ILogOutput> output) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outputs.push_back(std::move(output));
}

void Logger::RemoveAllOutputs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outputs.clear();
}

std::string Logger::FormatLogMessage(const std::string& message, LogLevel level) const {
    std::stringstream ss;
    ss << std::this_thread::get_id();

    return std::format("[{}][{}][{}] {}\n",
        GetTimeString(),
        GetLogLevelString(level),
        ss.str(),
        message);
}

std::string Logger::GetLogLevelString(LogLevel level) const
{
	switch (level) {
	case LogLevel::Debug:
		return "DEBUG";
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Warning:
		return "WARNING";
	case LogLevel::Error:
		return "ERROR";
	case LogLevel::Fatal:
		return "FATAL";
	default:
		return "UNKNOWN";
	}
}

std::string Logger::GetTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm;
    localtime_s(&tm, &time);

    return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
        tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());
}