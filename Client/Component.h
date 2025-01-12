#pragma once
#include "pch.h"

class GameObject;
class Component {
public:
    Component() = default;
    virtual ~Component() = default;

    // 복사 및 이동 생성자/대입 연산자 삭제
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

    // 생명주기 메서드
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Destroy() = 0;

    // 소유자 접근자
    GameObject* GetGameObject() const { return m_gameObject; }
    void SetGameObject(GameObject* gameObject) { m_gameObject = gameObject; }

    // 컴포넌트 활성화 상태 관리
    bool IsEnabled() const { return m_isEnabled; }
    virtual void Enable() { m_isEnabled = true; }
    virtual void Disable() { m_isEnabled = false; }

protected:
    GameObject* m_gameObject = nullptr;
    bool m_isEnabled = true;
};