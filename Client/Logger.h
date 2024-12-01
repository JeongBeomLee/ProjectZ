#pragma once
#include "pch.h"

// 로그 레벨 정의
enum class LogLevel {
    Debug,      // 개발용 상세 정보
    Info,       // 일반적인 정보
    Warning,    // 잠재적 문제
    Error,      // 심각한 문제
    Fatal       // 치명적 오류
};

// 로그 출력 인터페이스
class ILogOutput {
public:
    virtual ~ILogOutput() = default;
    virtual void Write(LogLevel level, const std::string& message) = 0;
};

// 디버그 창 출력
class DebugOutput : public ILogOutput {
public:
    void Write(LogLevel level, const std::string& message) override {
        OutputDebugStringA(message.c_str());
    }
};

// 파일 출력
class FileOutput : public ILogOutput {
public:
    explicit FileOutput(const std::string& filename) {
        m_file.open(filename, std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
    }
    ~FileOutput() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }
    void Write(LogLevel level, const std::string& message) override {
        if (m_file.is_open()) {
            m_file << message;
            m_file.flush();
        }
    }

private:
    std::ofstream m_file;
};

// 로거 클래스 (싱글톤)
class Logger {
public:
    static Logger& Instance();

    // 로그 출력 함수들
    template<typename... Args>
    void Debug(const std::string& format, Args... args);

    template<typename... Args>
    void Info(const std::string& format, Args... args);

    template<typename... Args>
    void Warning(const std::string& format, Args... args);

    template<typename... Args>
    void Error(const std::string& format, Args... args);

    template<typename... Args>
    void Fatal(const std::string& format, Args... args);

    // 출력 대상 추가/제거
    void AddOutput(std::unique_ptr<ILogOutput> output);
    void RemoveAllOutputs();

    // 복사/이동 방지
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;

    template<typename... Args>
    void Log(LogLevel level, const std::string& format, Args... args);

    std::string FormatLogMessage(const std::string& message, LogLevel level) const;
    std::string GetLogLevelString(LogLevel level) const;
    std::string GetTimeString() const;

    std::mutex m_mutex;
    std::vector<std::unique_ptr<ILogOutput>> m_outputs;
};

// 템플릿 함수 구현
template<typename... Args>
void Logger::Debug(const std::string& format, Args... args) {
    Log(LogLevel::Debug, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::Info(const std::string& format, Args... args) {
    Log(LogLevel::Info, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::Warning(const std::string& format, Args... args) {
    Log(LogLevel::Warning, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::Error(const std::string& format, Args... args) {
    Log(LogLevel::Error, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::Fatal(const std::string& format, Args... args) {
    Log(LogLevel::Fatal, format, std::forward<Args>(args)...);
}

template<typename... Args>
void Logger::Log(LogLevel level, const std::string& format, Args... args) {
    try {
        std::string message = std::vformat(format, std::make_format_args(args...));
        message = FormatLogMessage(message, level);

        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& output : m_outputs) {
            output->Write(level, message);
        }
    }
    catch (const std::exception& e) {
        // 포맷팅 실패 시 기본 에러 메시지 출력
        std::string errorMessage = std::format("Logging failed: {}", e.what());
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& output : m_outputs) {
            output->Write(LogLevel::Error, errorMessage);
        }
    }
}