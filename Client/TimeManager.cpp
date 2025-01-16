#include "pch.h"
#include "TimeManager.h"
#include "Logger.h"

TimeManager& TimeManager::Instance() {
    static TimeManager instance;
    return instance;
}

void TimeManager::Initialize() {
    m_prevTime = std::chrono::steady_clock::now();
    m_currentTime = m_prevTime;

    Logger::Instance().Info("TimeManager 초기화됨");
}

void TimeManager::Update() {
    m_currentTime = std::chrono::steady_clock::now();

    // 델타 타임 계산 (초 단위)
    std::chrono::duration<float> elapsed = m_currentTime - m_prevTime;
    m_unscaledDeltaTime = elapsed.count();

    // 델타 타임 보정
    constexpr float MAX_DELTA_TIME = 0.1f;  // 최대 100ms
    m_unscaledDeltaTime = std::min(m_unscaledDeltaTime, MAX_DELTA_TIME);

    // timeScale 적용
    m_deltaTime = m_unscaledDeltaTime * m_timeScale;

    // FPS 계산
    m_frameCount++;
    m_fpsTimer += m_unscaledDeltaTime;

    if (m_fpsTimer >= m_fpsUpdateInterval) {
        m_fps = m_frameCount / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer = 0;

        Logger::Instance().Debug("현재 FPS: {:.1f}", m_fps);
    }

    m_prevTime = m_currentTime;
}