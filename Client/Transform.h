#pragma once
#include "Component.h"

class Transform : public Component {
public:
    Transform();
    ~Transform() override = default;

    // Component 인터페이스 구현
    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

    // Transform 조작 메서드
    void SetPosition(const XMFLOAT3& position);
    void SetRotation(const XMFLOAT3& rotation);
    void SetScale(const XMFLOAT3& scale);

    XMFLOAT3 GetPosition() const { return m_position; }
    XMFLOAT3 GetRotation() const { return m_rotation; }
    XMFLOAT3 GetScale() const { return m_scale; }
    XMFLOAT3 GetForward() const;
	XMFLOAT3 GetRight() const;
	XMFLOAT3 GetUp() const;

	bool IsDirty() const { return m_isDirty; }

    // 변환 행렬 관련
    XMMATRIX GetWorldMatrix() const;
    void UpdateWorldMatrix();

    // 계층 구조 관련
    void SetParent(Transform* parent);
    Transform* GetParent() const { return m_parent; }
    const std::vector<Transform*>& GetChildren() const { return m_children; }

private:
    // 로컬 트랜스폼 정보
    XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };

    // 계층 구조
    Transform* m_parent = nullptr;
    std::vector<Transform*> m_children;

    // 캐시된 월드 변환 행렬
    XMMATRIX m_worldMatrix = XMMatrixIdentity();
    bool m_isDirty = true;  // 행렬 업데이트 필요 여부
};