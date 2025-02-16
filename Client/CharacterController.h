#pragma once
#include "Component.h"

class CharacterController : public Component {
public:
    CharacterController();
    ~CharacterController() override;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

    // 캐릭터 이동 제어 메서드
    void Move(const XMFLOAT3& displacement, float minDist, float deltaTime);
    void Jump(float jumpForce = 10.0f);

    // 상태 확인
    bool IsOnGround() const { return m_isOnGround; }
    const PxExtendedVec3& GetPosition() const;
    PxController* GetController() const { return m_controller; }

    // 설정
    void SetStepOffset(float offset);
    void SetSlopeLimit(float slopeLimit);
    void SetContactOffset(float offset);

private:
    void UpdateTransform();
    void HandleInput(float deltaTime);

    PxController* m_controller = nullptr;
    bool m_isOnGround = true;
    PxVec3 m_velocity = PxVec3(0.0f);
    float m_moveSpeed = 5.0f;
    float m_jumpForce = 10.0f;
    float m_gravity = -9.81f;
};