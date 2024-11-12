#pragma once
class PhysicsObject;
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
		float density = 1.0f);

	std::shared_ptr<PhysicsObject> CreateGroundPlane();

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

	// �ʱ�ȭ ���� �Լ���
	bool CreateFoundation();
	bool CreatePhysics();
	bool CreateScene();
	void SetupDebugger();
};

