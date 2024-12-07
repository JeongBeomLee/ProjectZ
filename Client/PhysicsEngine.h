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

	// ���� ��ü ���� �Լ���
	std::shared_ptr<PhysicsObject> CreateBox(
		const PxVec3& position,
		const PxVec3& dimensions,
		PhysicsObjectType type = PhysicsObjectType::DYNAMIC,
		CollisionGroup group = CollisionGroup::Default,      // �߰�
		CollisionGroup mask = CollisionGroup::Default,       // �߰�
		float density = 1.0f);

	std::shared_ptr<PhysicsObject> CreateGroundPlane();

	static PxFilterData CreateFilterData(CollisionGroup group, 
		CollisionGroup mask = CollisionGroup::Default);

	// ���� ��ü ����
	const std::vector<std::shared_ptr<PhysicsObject>>& GetPhysicsObjects() const { 
		return m_physicsObjects;
	}

	// PhysX �ֿ� ������Ʈ ������
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

	// ��ü �����̳�
	std::vector<std::shared_ptr<PhysicsObject>> m_physicsObjects;

	// �浹 �ݹ�
	std::unique_ptr<ContactReportCallback> m_contactCallback;

	// �ʱ�ȭ ���� �Լ���
	bool CreateFoundation();
	bool CreatePhysics();
	bool CreateScene();
	void SetupDebugger();

public:
	// �浹 ���͸� ������ ���� �޼���
	static PxFilterFlags CustomFilterShader(
		PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);
};

