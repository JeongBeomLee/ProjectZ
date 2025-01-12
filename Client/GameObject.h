#pragma once
#include "Component.h"
#include "Transform.h"

class GameObject {
public:
    GameObject();
    ~GameObject();

    // 생명주기 메서드
    void Initialize();
    void Update(float deltaTime);
    void Destroy();

    // 컴포넌트 관리
    template<typename T>
    T* AddComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");

        auto component = new T();
        component->SetGameObject(this);
        m_components.push_back(component);

        // Transform 컴포넌트인 경우 멤버 변수 업데이트
        if constexpr (std::is_same_v<T, Transform>) {
            m_transform = static_cast<Transform*>(component);
        }

        component->Initialize();
        return component;
    }

    template<typename T>
    T* GetComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");

        for (auto component : m_components) {
            if (T* result = dynamic_cast<T*>(component)) {
                return result;
            }
        }
        return nullptr;
    }

    // Transform 컴포넌트 빠른 접근
    Transform* GetTransform() const { return m_transform; }

    // 활성화 상태 관리
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active);

    // 고유 ID 접근자
    uint64_t GetID() const { return m_id; }

private:
    uint64_t m_id;  // 고유 ID
    bool m_isActive = true;
    Transform* m_transform = nullptr;  // Transform 컴포넌트 캐싱
    std::vector<Component*> m_components;

    static uint64_t s_nextId;  // ID 생성용 정적 변수
};