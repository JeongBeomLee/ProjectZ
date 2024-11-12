#pragma once
// ���� ��ü�� �������� �����ϴ� Ŭ����
class PhysicsObject {
public:
    PhysicsObject(PxRigidActor* actor);

    // ���� ��ü�� ��ȯ ����� DirectX ��ķ� ��ȯ
    XMMATRIX GetTransformMatrix() const;

    PxRigidActor* GetActor() const { return m_actor; }

private:
    PxRigidActor* m_actor;
};