#pragma once

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& GetInstance();

    // 로그 초기화 (파일 경로 설정)
    bool Initialize(const std::wstring& logFilePath);
    void Shutdown();

    // 로그 레벨 설정
    void SetLogLevel(LogLevel level) { m_logLevel = level; }

    // 로그 출력 함수들
    template<typename... Args>
    void Debug(const std::string_view format, const Args&... args);

    template<typename... Args>
    void Info(const std::string_view format, const Args&... args);

    template<typename... Args>
    void Warning(const std::string_view format, const Args&... args);

    template<typename... Args>
    void Error(const std::string_view format, const Args&... args);

    // 플러시 - 버퍼된 로그들을 강제로 파일에 쓰기
    void Flush();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename... Args>
    void LogMessage(LogLevel level, const std::string_view format,
        const std::source_location& location, const Args&... args);

    // 실제 로깅 수행 함수
    void WriteToFile(const std::wstring& message);
    void WriteToConsole(const std::wstring& message);

    // 로그 레벨을 문자열로 변환
    static const wchar_t* LogLevelToString(LogLevel level);

    // 현재 시간을 문자열로 반환
    static std::wstring GetTimeString();

private:
    std::mutex m_mutex;
    std::wofstream m_logFile;
    LogLevel m_logLevel = LogLevel::Debug;
    std::queue<std::wstring> m_logQueue;
    bool m_isInitialized = false;
};

// 템플릿 구현
template<typename... Args>
void Logger::Debug(const std::string_view format, const Args&... args) {
    if (m_logLevel <= LogLevel::Debug) {
        LogMessage(LogLevel::Debug, format, std::source_location::current(), args...);
    }
}

template<typename... Args>
void Logger::Info(const std::string_view format, const Args&... args) {
    if (m_logLevel <= LogLevel::Info) {
        LogMessage(LogLevel::Info, format, std::source_location::current(), args...);
    }
}

template<typename... Args>
void Logger::Warning(const std::string_view format, const Args&... args) {
    if (m_logLevel <= LogLevel::Warning) {
        LogMessage(LogLevel::Warning, format, std::source_location::current(), args...);
    }
}

template<typename... Args>
void Logger::Error(const std::string_view format, const Args&... args) {
    if (m_logLevel <= LogLevel::Error) {
        LogMessage(LogLevel::Error, format, std::source_location::current(), args...);
    }
}

template<typename... Args>
void Logger::LogMessage(LogLevel level, const std::string_view format,
    const std::source_location& location, const Args&... args) {
    try {
        // 메시지 포맷팅
        std::string message = std::vformat(format, std::make_format_args(args...));

        // 최종 로그 메시지 구성
        std::wstringstream ss;
        ss << GetTimeString() << L" "
            << LogLevelToString(level) << L" "
            << location.file_name() << L"(" << location.line() << L"): "
            << std::wstring(message.begin(), message.end());

        std::lock_guard<std::mutex> lock(m_mutex);
        m_logQueue.push(ss.str());

        // 에러 로그는 즉시 플러시
        if (level == LogLevel::Error) {
            Flush();
        }
    }
    catch (const std::exception& e) {
        // 로깅 실패 시 기본 출력
        OutputDebugStringA(("Logger error: " + std::string(e.what()) + "\n").c_str());
    }
}