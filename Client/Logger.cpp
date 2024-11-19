#include "pch.h"
#include "Logger.h"

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

bool Logger::Initialize(const std::wstring& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isInitialized) {
        return true;
    }

    try {
        m_logFile.open(logFilePath, std::ios::out | std::ios::app);
        if (!m_logFile.is_open()) {
            return false;
        }

        m_isInitialized = true;

        // 로그 시작 표시
        std::wstring startMessage = L"\n=== Log Started ===\n";
        WriteToFile(startMessage);
        WriteToConsole(startMessage);

        return true;
    }
    catch (const std::exception& e) {
        OutputDebugStringA(("Failed to initialize logger: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isInitialized) {
        Flush();

        // 로그 종료 표시
        std::wstring endMessage = L"\n=== Log Ended ===\n";
        WriteToFile(endMessage);
        WriteToConsole(endMessage);

        if (m_logFile.is_open()) {
            m_logFile.close();
        }

        m_isInitialized = false;
    }
}

void Logger::Flush() {
    // 이미 잠금된 상태에서 호출될 수 있으므로 중첩 잠금을 피하기 위한 체크
    const bool needLock = !m_mutex.try_lock();
    if (needLock) {
        m_mutex.lock();
    }

    while (!m_logQueue.empty()) {
        const std::wstring& message = m_logQueue.front();
        WriteToFile(message);
        WriteToConsole(message);
        m_logQueue.pop();
    }

    if (needLock) {
        m_mutex.unlock();
    }
}

void Logger::WriteToFile(const std::wstring& message) {
    if (m_logFile.is_open()) {
        m_logFile << message << std::endl;
        m_logFile.flush();
    }
}

void Logger::WriteToConsole(const std::wstring& message) {
    OutputDebugStringW(message.c_str());
    OutputDebugStringW(L"\n");
}

const wchar_t* Logger::LogLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:   return L"[DEBUG]";
    case LogLevel::Info:    return L"[INFO]";
    case LogLevel::Warning: return L"[WARNING]";
    case LogLevel::Error:   return L"[ERROR]";
    default:               return L"[UNKNOWN]";
    }
}

std::wstring Logger::GetTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::wstringstream ss;
    std::tm tm;
    localtime_s(&tm, &now_c);

    ss << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S")
        << L"." << std::setfill(L'0') << std::setw(3) << now_ms.count();

    return ss.str();
}