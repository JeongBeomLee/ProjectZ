#pragma once

class TimeManager {
public:
    static TimeManager& Instance();

    void Initialize();
    void Update();

    float GetDeltaTime() const { return m_deltaTime; }
    float GetTimeScale() const { return m_timeScale; }
    void SetTimeScale(float scale) { m_timeScale = scale; }

    // 실제 프레임 시간 (timeScale의 영향을 받지 않음)
    float GetUnscaledDeltaTime() const { return m_unscaledDeltaTime; }

    // 프레임 관련 정보
    float GetFPS() const { return m_fps; }
    float GetFrameCount() const { return m_frameCount; }

private:
    TimeManager() = default;
    ~TimeManager() = default;
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;

private:
    // 시간 측정을 위한 변수들
    std::chrono::steady_clock::time_point m_prevTime;
    std::chrono::steady_clock::time_point m_currentTime;

    // 델타 타임 관련
    float m_deltaTime = 0.0f;         // timeScale이 적용된 델타 타임
    float m_unscaledDeltaTime = 0.0f; // 실제 델타 타임
    float m_timeScale = 1.0f;         // 시간 배율 (기본값: 1.0)

    // FPS 계산을 위한 변수들
    float m_frameCount = 0.0f;
    float m_fps = 0.0f;
    float m_fpsUpdateInterval = 1.0f;  // FPS 업데이트 주기
    float m_fpsTimer = 0.0f;
};