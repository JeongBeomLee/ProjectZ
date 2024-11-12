#include "pch.h"
#include "PhysicsEngine.h"
#include "PhysicsObject.h"

PhysicsEngine::PhysicsEngine()
	: m_foundation(nullptr)
	, m_physics(nullptr)
	, m_dispatcher(nullptr)
	, m_scene(nullptr)
	, m_defaultMaterial(nullptr)
	, m_pvd(nullptr)
{
}

PhysicsEngine::~PhysicsEngine()
{
	Cleanup();
}

bool PhysicsEngine::Initialize()
{
	if (!CreateFoundation()) return false;
	SetupDebugger();
	if (!CreatePhysics()) return false;
	if (!CreateScene()) return false;

	// 기본 물리 material 생성
	m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.6f); // 동적 마찰, 정적 마찰, 반발력
	if (!m_defaultMaterial) return false;

	return true;
}

void PhysicsEngine::Update(float deltaTime)
{
	if (m_scene) {
		m_scene->simulate(deltaTime);
		m_scene->fetchResults(true);
	}
}

void PhysicsEngine::Cleanup()
{
	PX_RELEASE(m_scene);
	PX_RELEASE(m_dispatcher);
	PX_RELEASE(m_physics);
	if (m_pvd) {
		PxPvdTransport* transport = m_pvd->getTransport();
		PX_RELEASE(m_pvd);
		PX_RELEASE(transport);
	}
	PX_RELEASE(m_foundation);
}

std::shared_ptr<PhysicsObject> PhysicsEngine::CreateBox(
	const PxVec3& position, 
	const PxVec3& dimensions, 
	PhysicsObjectType type, 
	float density)
{
	PxRigidActor* actor = nullptr;

	switch (type)
	{
	case PhysicsObjectType::STATIC: {
		actor = m_physics->createRigidStatic(PxTransform(position));
		PxRigidActorExt::createExclusiveShape(*actor, PxBoxGeometry(dimensions), *m_defaultMaterial);
		break;
	}
	case PhysicsObjectType::DYNAMIC: {
		PxRigidDynamic* dynamicActor = m_physics->createRigidDynamic(PxTransform(position));
		PxRigidActorExt::createExclusiveShape(*dynamicActor, PxBoxGeometry(dimensions), *m_defaultMaterial);
		PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);
		actor = dynamicActor;
	}
	case PhysicsObjectType::KINEMATIC:
		break;
	default:
		break;
	}

	if (actor) {
		m_scene->addActor(*actor);
		auto physicsObject = std::make_shared<PhysicsObject>(actor);
		m_physicsObjects.push_back(physicsObject);
		return physicsObject;
	}

	return nullptr;
}

std::shared_ptr<PhysicsObject> PhysicsEngine::CreateGroundPlane()
{
	PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, PxPlane(0, 1, 0, 0), *m_defaultMaterial);
	if (groundPlane) {
		m_scene->addActor(*groundPlane);
		auto physicsObject = std::make_shared<PhysicsObject>(groundPlane);
		m_physicsObjects.push_back(physicsObject);
		return physicsObject;
	}
	return nullptr;
}

bool PhysicsEngine::CreateFoundation()
{
	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
	return m_foundation != nullptr;
}

bool PhysicsEngine::CreatePhysics()
{
	PxTolerancesScale scale;
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true, m_pvd);
	return m_physics != nullptr;
}

bool PhysicsEngine::CreateScene()
{
	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

	// CPU 디스패처 설정 (멀티스레딩)
	m_dispatcher = PxDefaultCpuDispatcherCreate(2);  // 2개의 워커 스레드
	if (!m_dispatcher) return false;
	sceneDesc.cpuDispatcher = m_dispatcher;

	// 필터 셰이더 설정 (충돌 필터링)
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;

	m_scene = m_physics->createScene(sceneDesc);
	return m_scene != nullptr;
}

void PhysicsEngine::SetupDebugger()
{
	// PhysX Visual Debugger 설정
	m_pvd = PxCreatePvd(*m_foundation);
	if (m_pvd) {
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
		m_pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	}
}