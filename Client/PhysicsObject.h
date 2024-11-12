#pragma once
// 물리 객체와 렌더링을 연결하는 클래스
class PhysicsObject {
public:
    PhysicsObject(PxRigidActor* actor);

    // 물리 객체의 변환 행렬을 DirectX 행렬로 변환
    XMMATRIX GetTransformMatrix() const;

    PxRigidActor* GetActor() const { return m_actor; }

private:
    PxRigidActor* m_actor;
};