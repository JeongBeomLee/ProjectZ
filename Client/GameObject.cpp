#include "pch.h"
#include "GameObject.h"
#include "Logger.h"

uint64_t GameObject::s_nextId = 0;

GameObject::GameObject() : m_id(s_nextId++)
{
    // GameObject 생성 시 Transform 컴포넌트는 자동으로 추가
    AddComponent<Transform>();
    Logger::Instance().Debug("GameObject 생성됨. ID: {}", m_id);
}

GameObject::~GameObject()
{
    Destroy();
    Logger::Instance().Debug("GameObject 제거됨. ID: {}", m_id);
}

void GameObject::Initialize()
{
    if (!m_isActive) return;

    // Transform은 이미 Initialize되었으므로 제외
    for (auto component : m_components) {
        if (component != m_transform) {
            component->Initialize();
        }
    }
}

void GameObject::Update(float deltaTime)
{
    if (!m_isActive) return;

    // 모든 컴포넌트 업데이트
    for (auto component : m_components) {
        if (component->IsEnabled()) {
            component->Update(deltaTime);
        }
    }
}

void GameObject::Destroy()
{
    // 모든 컴포넌트 정리
    for (auto component : m_components) {
        component->Destroy();
        delete component;
    }
    m_components.clear();
    m_transform = nullptr;
}

void GameObject::SetActive(bool active)
{
    if (m_isActive == active) return;

    m_isActive = active;
    Logger::Instance().Debug("GameObject {} {}됨. ID: {}",
        m_id, active ? "활성화" : "비활성화", m_id);

    // 모든 컴포넌트의 활성화 상태도 변경
    for (auto component : m_components) {
        if (active) {
            component->Enable();
        }
        else {
            component->Disable();
        }
    }
}