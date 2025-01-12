#include "pch.h"
#include "Transform.h"
#include "GameObject.h"
//#include "Logger.h"

void Transform::Initialize()
{
    UpdateWorldMatrix();
}

void Transform::Update(float deltaTime)
{
    if (m_isDirty) {
        UpdateWorldMatrix();
    }
}

void Transform::Destroy()
{
    // 부모로부터 분리
    SetParent(nullptr);

    // 모든 자식도 부모로부터 분리
    while (!m_children.empty()) {
        m_children[0]->SetParent(nullptr);
    }
}

void Transform::SetPosition(const XMFLOAT3& position)
{
    m_position = position;
    m_isDirty = true;
}

void Transform::SetRotation(const XMFLOAT3& rotation)
{
    m_rotation = rotation;
    m_isDirty = true;
}

void Transform::SetScale(const XMFLOAT3& scale)
{
    m_scale = scale;
    m_isDirty = true;
}

XMMATRIX Transform::GetWorldMatrix() const
{
    if (m_isDirty) {
        const_cast<Transform*>(this)->UpdateWorldMatrix();
    }
    return m_worldMatrix;
}

void Transform::UpdateWorldMatrix()
{
    // 로컬 변환 행렬 계산
    XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(m_rotation.x),
        XMConvertToRadians(m_rotation.y),
        XMConvertToRadians(m_rotation.z));
    XMMATRIX scale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

    // SRT
    XMMATRIX localMatrix = scale * rotation * translation;

    // 부모가 있다면 부모의 월드 행렬과 곱하기
    m_worldMatrix = m_parent ? localMatrix * m_parent->GetWorldMatrix() : localMatrix;
    m_isDirty = false;

    // 자식들의 월드 행렬도 업데이트
    for (auto child : m_children) {
        child->m_isDirty = true;
    }
}

void Transform::SetParent(Transform* parent)
{
    if (m_parent == parent) return;

    // 이전 부모로부터 제거
    if (m_parent) {
        auto& siblings = m_parent->m_children;
        siblings.erase(
            std::remove(siblings.begin(), siblings.end(), this),
            siblings.end()
        );
    }

    // 새로운 부모 설정
    m_parent = parent;

    // 새로운 부모의 자식으로 추가
    if (m_parent) {
        m_parent->m_children.push_back(this);
    }

    // 월드 행렬 업데이트
    m_isDirty = true;
}