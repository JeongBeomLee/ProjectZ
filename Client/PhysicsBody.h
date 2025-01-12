#pragma once
#include "Component.h"
#include "PhysicsTypes.h"
#include "PhysicsObject.h"

class PhysicsBody : public Component {
public:
    struct BoxParams {
        PxVec3 dimensions = PxVec3(1.0f);
    };

    struct SphereParams {
        float radius = 1.0f;
    };

    struct CapsuleParams {
        float radius = 0.5f;
        float halfHeight = 1.0f;
    };

    struct MeshParams {
        std::vector<PxVec3> vertices;
        std::vector<uint32_t> indices;
    };

    PhysicsBody() = default;
    ~PhysicsBody() override;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

	// 물리 객체 생성 메서드 (박스, 구, 캡슐, 메시)
    bool CreateBody(PhysicsObjectType type,
        PhysicsShapeType shape,
		const BoxParams& params,    // 박스 생성
        float density = 1.0f);

    bool CreateBody(PhysicsObjectType type,
        PhysicsShapeType shape,
		const SphereParams& params, // 구 생성
        float density = 1.0f);

    bool CreateBody(PhysicsObjectType type,
        PhysicsShapeType shape,
		const CapsuleParams& params,// 캡슐 생성
        float density = 1.0f);

    bool CreateBody(PhysicsObjectType type,
        PhysicsShapeType shape,
		const MeshParams& params); // 메시 생성

    // 물리 객체 조작 메서드
    void AddForce(const PxVec3& force, PxForceMode::Enum mode = PxForceMode::eFORCE);
    void AddTorque(const PxVec3& torque, PxForceMode::Enum mode = PxForceMode::eFORCE);
    void SetLinearVelocity(const PxVec3& velocity);
    void SetAngularVelocity(const PxVec3& velocity);

    // 충돌 그룹 설정
    void SetCollisionGroup(CollisionGroup group) { m_group = group; }
    void SetCollisionMask(CollisionGroup mask) { m_mask = mask; }

    // 물리 객체 상태 확인
    bool IsKinematic() const;
    bool IsDynamic() const;
    bool IsStatic() const;

private:
    void UpdatePhysicsTransform();  // 물리 엔진의 변환 행렬을 가져와서 Transform 업데이트

    std::shared_ptr<PhysicsObject> m_physicsObject;
    CollisionGroup m_group = CollisionGroup::Default;
    CollisionGroup m_mask = CollisionGroup::Default;
    PhysicsShapeType m_shapeType;
};