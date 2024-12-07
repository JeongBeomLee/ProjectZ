#include "pch.h"
#include "PhysicsEngine.h"
#include "PhysicsObject.h"
#include "ContactReportCallback.h"
#include "Logger.h"

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
		try {
			// simulate 호출 전 상태 체크
			m_scene->simulate(deltaTime);

			// simulate의 결과를 기다림
			m_scene->fetchResults(true);
		}
		catch (const std::exception& e) {
			Logger::Instance().Error("PhysicsEngine::Update - Exception: {}", e.what());
		}
	}
	else {
		Logger::Instance().Error("PhysicsEngine::Update - Scene is null");
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
	CollisionGroup group, CollisionGroup mask, 
	float density)
{
	PxRigidActor* actor = nullptr;
	PxShape* shape = nullptr;

	switch (type)
	{
	case PhysicsObjectType::STATIC: {
		actor = m_physics->createRigidStatic(PxTransform(position));
		actor->setName("StaticBox");
		shape = PxRigidActorExt::createExclusiveShape(*actor,
			PxBoxGeometry(dimensions), *m_defaultMaterial);
		break;
	}
	case PhysicsObjectType::DYNAMIC: {
		PxRigidDynamic* dynamicActor = m_physics->createRigidDynamic(PxTransform(position));
		dynamicActor->setName("DynamicBox");
		shape = PxRigidActorExt::createExclusiveShape(*dynamicActor,
			PxBoxGeometry(dimensions), *m_defaultMaterial);
		PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);
		actor = dynamicActor;
		break;
	}
	case PhysicsObjectType::KINEMATIC:
		break;
	default:
		break;
	}

	if (actor && shape) {
		// 충돌 필터 데이터 설정
		shape->setSimulationFilterData(CreateFilterData(group, mask));

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
		// 충돌 필터 데이터 설정
		m_scene->addActor(*groundPlane);
		auto physicsObject = std::make_shared<PhysicsObject>(groundPlane);
		m_physicsObjects.push_back(physicsObject);
		return physicsObject;
	}
	return nullptr;
}

PxFilterData PhysicsEngine::CreateFilterData(CollisionGroup group, CollisionGroup mask)
{
	PxFilterData filterData;
	filterData.word0 = static_cast<PxU32>(group);  // 자신의 그룹
	filterData.word1 = static_cast<PxU32>(mask);   // 충돌할 그룹
	return filterData;
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

	// 충돌 콜백 생성 및 설정
	m_contactCallback = std::make_unique<ContactReportCallback>();
	sceneDesc.simulationEventCallback = m_contactCallback.get();

	// 커스텀 필터 셰이더 설정
	sceneDesc.filterShader = CustomFilterShader;

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

PxFilterFlags PhysicsEngine::CustomFilterShader(
	PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
	PxFilterObjectAttributes attributes1, PxFilterData filterData1, 
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// 트리거 객체 처리
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}

	// 충돌 마스크 확인
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1)) {
		pairFlags = PxPairFlag::eCONTACT_DEFAULT  // 기본 충돌 속성
			| PxPairFlag::eNOTIFY_TOUCH_FOUND     // 충돌 시작 알림
			| PxPairFlag::eNOTIFY_TOUCH_LOST      // 충돌 종료 알림
			| PxPairFlag::eNOTIFY_CONTACT_POINTS; // 접촉점 정보 제공

		return PxFilterFlag::eDEFAULT;
	}

	return PxFilterFlag::eSUPPRESS; // 충돌 무시
}
