#include "pch.h"
#include "PhysicsBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Engine.h"
#include "PhysicsEngine.h"
#include "Logger.h"

PhysicsBody::~PhysicsBody() {
    Destroy();
}

void PhysicsBody::Initialize() {
    Logger::Instance().Debug("PhysicsBody 초기화됨");
}

void PhysicsBody::Update(float deltaTime) {
    if (!m_physicsObject || !IsEnabled()) return;

    if (IsDynamic() || IsKinematic()) {
        UpdatePhysicsTransform();
    }
}

void PhysicsBody::Destroy() {
    m_physicsObject.reset();
}

bool PhysicsBody::CreateBody(PhysicsObjectType type,
    PhysicsShapeType shape,
    const BoxParams& params,
    float density) {
    if (shape != PhysicsShapeType::Box) {
        Logger::Instance().Error("잘못된 형상 타입이 지정됨: Box 필요");
        return false;
    }

    auto transform = GetGameObject()->GetTransform();
    auto position = transform->GetPosition();

    m_physicsObject = Engine::Instance().GetPhysicsEngine()->CreateBox(
        PxVec3(position.x, position.y, position.z),
        params.dimensions,
        type,
        m_group,
        m_mask,
        density
    );

    if (m_physicsObject) {
        m_shapeType = shape;
        Logger::Instance().Debug("Box 물리 객체 생성됨");
        return true;
    }

    return false;
}

bool PhysicsBody::CreateBody(PhysicsObjectType type,
    PhysicsShapeType shape,
    const SphereParams& params,
    float density) {
    if (shape != PhysicsShapeType::Sphere) {
        Logger::Instance().Error("잘못된 형상 타입이 지정됨: Sphere 필요");
        return false;
    }

    auto transform = GetGameObject()->GetTransform();
    auto position = transform->GetPosition();

    m_physicsObject = Engine::Instance().GetPhysicsEngine()->CreateSphere(
        PxVec3(position.x, position.y, position.z),
        params.radius,
        type,
        m_group,
        m_mask,
        density
    );

    if (m_physicsObject) {
        m_shapeType = shape;
        Logger::Instance().Debug("Sphere 물리 객체 생성됨");
        return true;
    }

    return false;
}

bool PhysicsBody::CreateBody(PhysicsObjectType type,
    PhysicsShapeType shape,
    const CapsuleParams& params,
    float density) {
    if (shape != PhysicsShapeType::Capsule) {
        Logger::Instance().Error("잘못된 형상 타입이 지정됨: Capsule 필요");
        return false;
    }

    auto transform = GetGameObject()->GetTransform();
    auto position = transform->GetPosition();

    m_physicsObject = Engine::Instance().GetPhysicsEngine()->CreateCapsule(
        PxVec3(position.x, position.y, position.z),
        params.radius,
        params.halfHeight,
        type,
        m_group,
        m_mask,
        density
    );

    if (m_physicsObject) {
        m_shapeType = shape;
        Logger::Instance().Debug("Capsule 물리 객체 생성됨");
        return true;
    }

    return false;
}

bool PhysicsBody::CreateBody(PhysicsObjectType type,
    PhysicsShapeType shape,
    const MeshParams& params) {
    if (shape != PhysicsShapeType::TriangleMesh) {
        Logger::Instance().Error("잘못된 형상 타입이 지정됨: TriangleMesh 필요");
        return false;
    }

    auto transform = GetGameObject()->GetTransform();
    auto position = transform->GetPosition();

    m_physicsObject = Engine::Instance().GetPhysicsEngine()->CreateTriangleMesh(
        PxVec3(position.x, position.y, position.z),
        params.vertices,
        params.indices,
        type,
        m_group,
        m_mask
    );

    if (m_physicsObject) {
        m_shapeType = shape;
        Logger::Instance().Debug("TriangleMesh 물리 객체 생성됨");
        return true;
    }

    return false;
}

void PhysicsBody::UpdatePhysicsTransform() {
    if (!m_physicsObject) return;

    // PhysX의 변환 행렬을 GameObject의 Transform에 적용
    auto worldMatrix = m_physicsObject->GetTransformMatrix();

    // 위치, 회전, 크기 추출
    XMVECTOR scale, rotationQuat, translation;
    XMMatrixDecompose(&scale, &rotationQuat, &translation, worldMatrix);

    // Transform 컴포넌트 업데이트
    auto transform = GetGameObject()->GetTransform();

    XMFLOAT3 position;
    XMStoreFloat3(&position, translation);
    transform->SetPosition(position);

    // 쿼터니온을 오일러 각으로 변환
    XMFLOAT4 quat;
    XMStoreFloat4(&quat, rotationQuat);
    float pitch = asinf(-2.0f * (quat.x * quat.z - quat.w * quat.y));
    float yaw = atan2f(2.0f * (quat.x * quat.y + quat.w * quat.z),
        quat.w * quat.w + quat.x * quat.x - quat.y * quat.y - quat.z * quat.z);
    float roll = atan2f(2.0f * (quat.y * quat.z + quat.w * quat.x),
        quat.w * quat.w - quat.x * quat.x - quat.y * quat.y + quat.z * quat.z);

    transform->SetRotation(XMFLOAT3(
        XMConvertToDegrees(pitch),
        XMConvertToDegrees(yaw),
        XMConvertToDegrees(roll)
    ));

    XMFLOAT3 scaleFloat;
    XMStoreFloat3(&scaleFloat, scale);
    transform->SetScale(scaleFloat);
}

void PhysicsBody::AddForce(const PxVec3& force, PxForceMode::Enum mode) {
    if (!m_physicsObject || !IsDynamic()) return;

    auto actor = static_cast<PxRigidDynamic*>(m_physicsObject->GetActor());
    actor->addForce(force, mode);
}

void PhysicsBody::AddTorque(const PxVec3& torque, PxForceMode::Enum mode) {
    if (!m_physicsObject || !IsDynamic()) return;

    auto actor = static_cast<PxRigidDynamic*>(m_physicsObject->GetActor());
    actor->addTorque(torque, mode);
}

void PhysicsBody::SetLinearVelocity(const PxVec3& velocity) {
    if (!m_physicsObject || !IsDynamic()) return;

    auto actor = static_cast<PxRigidDynamic*>(m_physicsObject->GetActor());
    actor->setLinearVelocity(velocity);
}

void PhysicsBody::SetAngularVelocity(const PxVec3& velocity) {
    if (!m_physicsObject || !IsDynamic()) return;

    auto actor = static_cast<PxRigidDynamic*>(m_physicsObject->GetActor());
    actor->setAngularVelocity(velocity);
}

bool PhysicsBody::IsKinematic() const {
    if (!m_physicsObject) return false;
    auto actor = m_physicsObject->GetActor();
    return actor->is<PxRigidDynamic>() && static_cast<PxRigidDynamic*>(actor)->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
}

bool PhysicsBody::IsDynamic() const {
    if (!m_physicsObject) return false;
    return m_physicsObject->GetActor()->is<PxRigidDynamic>();
}

bool PhysicsBody::IsStatic() const {
    if (!m_physicsObject) return false;
    return m_physicsObject->GetActor()->is<PxRigidStatic>();
}