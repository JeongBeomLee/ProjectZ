#pragma once

class InputManager {
public:
    static InputManager& Instance();

    void Initialize(HWND hwnd);
    void Update();

    // 키보드 상태 확인
    bool IsKeyDown(int keyCode) const;
    bool IsKeyUp(int keyCode) const;
    bool IsKeyPressed(int keyCode) const;  // 이번 프레임에 눌림
    bool IsKeyReleased(int keyCode) const; // 이번 프레임에 떼짐

    // 마우스 상태 확인
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonUp(int button) const;
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonReleased(int button) const;

    // 마우스 위치 및 이동량
    const POINT& GetMousePosition() const { return m_mousePosition; }
    const POINT& GetMouseDelta() const { return m_mouseDelta; }

    // 마우스 이동 모드
    void SetMouseMode(bool relative) { m_relativeMouseMode = relative; }
    bool IsRelativeMouseMode() const { return m_relativeMouseMode; }
    void ProcessMouseMove(int x, int y);

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void UpdateKeyStates();
    void UpdateMouseButtonStates();

private:
    HWND m_hwnd = nullptr;

    // 키보드 상태
    std::array<bool, 256> m_currentKeyStates = {};
    std::array<bool, 256> m_previousKeyStates = {};

    // 마우스 버튼 상태
    std::array<bool, 3> m_currentMouseStates = {};
    std::array<bool, 3> m_previousMouseStates = {};

    // 마우스 위치 정보
    POINT m_mousePosition = {};
    POINT m_previousMousePosition = {};
    POINT m_mouseDelta = {};
    bool m_relativeMouseMode = false;

    // 윈도우 중앙점 (상대 마우스 모드용)
    POINT m_windowCenter = {};
};