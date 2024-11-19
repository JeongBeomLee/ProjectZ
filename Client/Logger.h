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

    // �α� �ʱ�ȭ (���� ��� ����)
    bool Initialize(const std::wstring& logFilePath);
    void Shutdown();

    // �α� ���� ����
    void SetLogLevel(LogLevel level) { m_logLevel = level; }

    // �α� ��� �Լ���
    template<typename... Args>
    void Debug(const std::string_view format, const Args&... args);

    template<typename... Args>
    void Info(const std::string_view format, const Args&... args);

    template<typename... Args>
    void Warning(const std::string_view format, const Args&... args);

    template<typename... Args>
    void Error(const std::string_view format, const Args&... args);

    // �÷��� - ���۵� �α׵��� ������ ���Ͽ� ����
    void Flush();

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename... Args>
    void LogMessage(LogLevel level, const std::string_view format,
        const std::source_location& location, const Args&... args);

    // ���� �α� ���� �Լ�
    void WriteToFile(const std::wstring& message);
    void WriteToConsole(const std::wstring& message);

    // �α� ������ ���ڿ��� ��ȯ
    static const wchar_t* LogLevelToString(LogLevel level);

    // ���� �ð��� ���ڿ��� ��ȯ
    static std::wstring GetTimeString();

private:
    std::mutex m_mutex;
    std::wofstream m_logFile;
    LogLevel m_logLevel = LogLevel::Debug;
    std::queue<std::wstring> m_logQueue;
    bool m_isInitialized = false;
};

// ���ø� ����
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
        // �޽��� ������
        std::string message = std::vformat(format, std::make_format_args(args...));

        // ���� �α� �޽��� ����
        std::wstringstream ss;
        ss << GetTimeString() << L" "
            << LogLevelToString(level) << L" "
            << location.file_name() << L"(" << location.line() << L"): "
            << std::wstring(message.begin(), message.end());

        std::lock_guard<std::mutex> lock(m_mutex);
        m_logQueue.push(ss.str());

        // ���� �α״� ��� �÷���
        if (level == LogLevel::Error) {
            Flush();
        }
    }
    catch (const std::exception& e) {
        // �α� ���� �� �⺻ ���
        OutputDebugStringA(("Logger error: " + std::string(e.what()) + "\n").c_str());
    }
}