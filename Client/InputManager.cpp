#include "pch.h"
#include "InputManager.h"
#include "Logger.h"

InputManager& InputManager::Instance() 
{
    static InputManager instance;
    return instance;
}

void InputManager::Initialize(HWND hwnd) 
{
    m_hwnd = hwnd;

    // 윈도우 중앙점 계산
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    m_windowCenter.x = (windowRect.right - windowRect.left) / 2;
    m_windowCenter.y = (windowRect.bottom - windowRect.top) / 2;

    Logger::Instance().Info("InputManager 초기화됨");
}

void InputManager::Update() 
{
    // 이전 상태 저장
    m_previousKeyStates = m_currentKeyStates;
    m_previousMouseStates = m_currentMouseStates;
    m_previousMousePosition = m_mousePosition;

    // 키보드 상태 업데이트
    UpdateKeyStates();

    // 마우스 버튼 상태 업데이트
    UpdateMouseButtonStates();

    // 상대 마우스 모드인 경우 마우스를 윈도우 중앙으로 강제 이동
    if (m_relativeMouseMode) {
        POINT center = m_windowCenter;
        ClientToScreen(m_hwnd, &center);
        SetCursorPos(center.x, center.y);
    }
}

void InputManager::UpdateKeyStates() 
{
    // 모든 키의 상태 검사
    for (int i = 0; i < 256; ++i) {
        m_currentKeyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }
}

void InputManager::UpdateMouseButtonStates() 
{
    m_currentMouseStates[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;  // 좌클릭
    m_currentMouseStates[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;  // 우클릭
    m_currentMouseStates[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;  // 중간 버튼
}

void InputManager::ProcessMouseMove(int x, int y) 
{
    // 현재 마우스 위치 저장
    m_mousePosition = { x, y };

    if (m_relativeMouseMode) {
        // 상대 모드: 중앙점으로부터의 델타 계산
        m_mouseDelta.x = x - m_windowCenter.x;
        m_mouseDelta.y = y - m_windowCenter.y;
    }
    else {
        // 일반 모드: 이전 위치로부터의 델타 계산
        m_mouseDelta.x = x - m_previousMousePosition.x;
        m_mouseDelta.y = y - m_previousMousePosition.y;
    }
}

bool InputManager::IsKeyDown(int keyCode) const 
{
    return m_currentKeyStates[keyCode];
}

bool InputManager::IsKeyUp(int keyCode) const 
{
    return !m_currentKeyStates[keyCode];
}

bool InputManager::IsKeyPressed(int keyCode) const 
{
    return m_currentKeyStates[keyCode] && !m_previousKeyStates[keyCode];
}

bool InputManager::IsKeyReleased(int keyCode) const 
{
    return !m_currentKeyStates[keyCode] && m_previousKeyStates[keyCode];
}

bool InputManager::IsMouseButtonDown(int button) const
{
    return m_currentMouseStates[button];
}

bool InputManager::IsMouseButtonUp(int button) const 
{
    return !m_currentMouseStates[button];
}

bool InputManager::IsMouseButtonPressed(int button) const 
{
    return m_currentMouseStates[button] && !m_previousMouseStates[button];
}

bool InputManager::IsMouseButtonReleased(int button) const 
{
    return !m_currentMouseStates[button] && m_previousMouseStates[button];
}