#pragma once
#include "PhysicsTypes.h"
class PhysicsObject;
class ContactReportCallback;
class PhysicsEngine
{
public:
	PhysicsEngine();
	~PhysicsEngine();

	bool Initialize();
	void Update(float deltaTime);
	void Cleanup();

	// 물리 객체 생성 함수들
	std::shared_ptr<PhysicsObject> CreateBox(
		const PxVec3& position,
		const PxVec3& dimensions,
		PhysicsObjectType type = PhysicsObjectType::DYNAMIC,
		CollisionGroup group = CollisionGroup::Default,      // 추가
		CollisionGroup mask = CollisionGroup::Default,       // 추가
		float density = 1.0f);

	std::shared_ptr<PhysicsObject> CreateGroundPlane();

	static PxFilterData CreateFilterData(CollisionGroup group, 
		CollisionGroup mask = CollisionGroup::Default);

	// 물리 객체 관리
	const std::vector<std::shared_ptr<PhysicsObject>>& GetPhysicsObjects() const { 
		return m_physicsObjects;
	}

	// PhysX 주요 컴포넌트 접근자
	PxPhysics* GetPhysics() const { return m_physics; }
	PxScene* GetScene() const { return m_scene; }

private:
	PxDefaultAllocator m_allocator;
	PxDefaultErrorCallback m_errorCallback;
	PxFoundation* m_foundation;
	PxPhysics* m_physics;
	PxDefaultCpuDispatcher* m_dispatcher;
	PxScene* m_scene;
	PxMaterial* m_defaultMaterial;
	PxPvd* m_pvd; // PhysX Visual Debugger

	// 객체 컨테이너
	std::vector<std::shared_ptr<PhysicsObject>> m_physicsObjects;

	// 충돌 콜백
	std::unique_ptr<ContactReportCallback> m_contactCallback;

	// 초기화 헬퍼 함수들
	bool CreateFoundation();
	bool CreatePhysics();
	bool CreateScene();
	void SetupDebugger();

public:
	// 충돌 필터링 설정을 위한 메서드
	static PxFilterFlags CustomFilterShader(
		PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);
};

