#pragma once
#include "pch.h"

class GameObject;
class Component {
public:
    enum class UpdatePriority {
        Default = 0,           // 기본 우선순위
        Camera = 50,          // 카메라는 Transform보다 먼저 업데이트
        Physics = 80,         // 물리는 Transform보다 먼저 업데이트
        Transform = 100,      // Transform은 중간 정도의 우선순위
        Renderer = 150        // 렌더러는 Transform 이후에 업데이트
    };

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

    // 우선순위 관련 메서드 추가
    void SetUpdatePriority(UpdatePriority priority) { m_updatePriority = priority; }
    UpdatePriority GetUpdatePriority() const { return m_updatePriority; }

protected:
    GameObject* m_gameObject = nullptr;
    bool m_isEnabled = true;
	UpdatePriority m_updatePriority = UpdatePriority::Default;
};