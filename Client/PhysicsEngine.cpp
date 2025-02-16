#include "pch.h"
#include "PhysicsEngine.h"
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
	m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.4f); // 동적 마찰, 정적 마찰, 반발력
	if (!m_defaultMaterial) return false;

	// 컨트롤러 매니저 생성
	m_controllerHitReport = std::make_unique<CharacterControllerHitReport>();
	m_controllerManager = PxCreateControllerManager(*m_scene);
	if (!m_controllerManager) {
		Logger::Instance().Error("컨트롤러 매니저 생성 실패");
		return false;
	}

	// 장애물 컨텍스트 생성
	m_obstacleContext = m_controllerManager->createObstacleContext();
	if (!m_obstacleContext) {
		Logger::Instance().Error("장애물 컨텍스트 생성 실패");
		return false;
	}

	return true;
}

void PhysicsEngine::Update(float deltaTime)
{
	if (m_scene) {
		try {
			// 장애물 업데이트 (지금은 계속 호출하도록)
			UpdateObstacles();

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

void PhysicsEngine::UpdateObstacles()
{
	if (!m_obstacleContext) return;

	// 장애물 업데이트
	for (const auto& [handle, obstacle] : m_obstacles) {
		m_obstacleContext->updateObstacle(handle, *obstacle);
	}
}

void PhysicsEngine::Cleanup()
{
	// 장애물 정리
	ClearObstacles();

	// 장애물 컨텍스트 정리
	PX_RELEASE(m_obstacleContext);

	m_contactCallback.reset();
	m_controllerHitReport.reset();

	// 컨트롤러 매니저 정리
	m_controllerManager->purgeControllers();
	PX_RELEASE(m_controllerManager);

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

	switch (type) {
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
	default:
		break;
	}

	if (actor && shape) {
		// 충돌 필터 데이터 설정
		shape->setSimulationFilterData(CreateFilterData(group, mask));
		shape->setQueryFilterData(CreateFilterData(group, mask));
		m_scene->addActor(*actor);
		auto physicsObject = std::make_shared<PhysicsObject>(actor);
		m_physicsObjects.push_back(physicsObject);
		return physicsObject;
	}

	return nullptr;
}

std::shared_ptr<PhysicsObject> PhysicsEngine::CreateSphere(
	const PxVec3& position, float radius, PhysicsObjectType type,
	CollisionGroup group, CollisionGroup mask, float density) {

	PxRigidActor* actor = nullptr;
	PxShape* shape = nullptr;

	switch (type) {
	case PhysicsObjectType::STATIC: {
		actor = m_physics->createRigidStatic(PxTransform(position));
		actor->setName("StaticSphere");
		shape = PxRigidActorExt::createExclusiveShape(*actor,
			PxSphereGeometry(radius), *m_defaultMaterial);
		break;
	}
	case PhysicsObjectType::DYNAMIC: {
		PxRigidDynamic* dynamicActor = m_physics->createRigidDynamic(PxTransform(position));
		dynamicActor->setName("DynamicSphere");
		shape = PxRigidActorExt::createExclusiveShape(*dynamicActor,
			PxSphereGeometry(radius), *m_defaultMaterial);
		PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);
		actor = dynamicActor;
		break;
	}
	}

	if (actor && shape) {
		shape->setSimulationFilterData(CreateFilterData(group, mask));
		shape->setQueryFilterData(CreateFilterData(group, mask));
		m_scene->addActor(*actor);
		auto physicsObject = std::make_shared<PhysicsObject>(actor);
		m_physicsObjects.push_back(physicsObject);
		return physicsObject;
	}

	return nullptr;
}

std::shared_ptr<PhysicsObject> PhysicsEngine::CreateCapsule(
	const PxVec3& position, float radius, float halfHeight, PhysicsObjectType type,
	CollisionGroup group, CollisionGroup mask, float density) {

	PxRigidActor* actor = nullptr;
	PxShape* shape = nullptr;

	switch (type) {
	case PhysicsObjectType::STATIC: {
		actor = m_physics->createRigidStatic(PxTransform(position));
		actor->setName("StaticCapsule");
		shape = PxRigidActorExt::createExclusiveShape(*actor,
			PxCapsuleGeometry(radius, halfHeight), *m_defaultMaterial);
		break;
	}
	case PhysicsObjectType::DYNAMIC: {
		PxRigidDynamic* dynamicActor = m_physics->createRigidDynamic(PxTransform(position));
		dynamicActor->setName("DynamicCapsule");
		shape = PxRigidActorExt::createExclusiveShape(*dynamicActor,
			PxCapsuleGeometry(radius, halfHeight), *m_defaultMaterial);
		PxRigidBodyExt::updateMassAndInertia(*dynamicActor, density);
		actor = dynamicActor;
		break;
	}
	}

	if (actor && shape) {
		shape->setSimulationFilterData(CreateFilterData(group, mask));
		shape->setQueryFilterData(CreateFilterData(group, mask));
		m_scene->addActor(*actor);
		auto physicsObject = std::make_shared<PhysicsObject>(actor);
		m_physicsObjects.push_back(physicsObject);
		return physicsObject;
	}

	return nullptr;
}

std::shared_ptr<PhysicsObject> PhysicsEngine::CreateTriangleMesh(
	const PxVec3& position,
	const std::vector<PxVec3>& vertices,
	const std::vector<uint32_t>& indices,
	PhysicsObjectType type,
	CollisionGroup group,
	CollisionGroup mask) {

	// 메시 데이터 생성
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = static_cast<PxU32>(vertices.size());
	meshDesc.points.stride = sizeof(PxVec3);
	meshDesc.points.data = vertices.data();
	meshDesc.triangles.count = static_cast<PxU32>(indices.size() / 3);
	meshDesc.triangles.stride = 3 * sizeof(uint32_t);
	meshDesc.triangles.data = indices.data();

	PxTolerancesScale scale;
	PxCookingParams params(scale);

	// 메시 쿠킹
	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	bool status = PxCookTriangleMesh(params, meshDesc, writeBuffer, &result);
	if (!status) return nullptr;

	// 쿠킹된 데이터로 메시 생성
	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	PxTriangleMesh* triangleMesh = m_physics->createTriangleMesh(readBuffer);
	if (!triangleMesh) return nullptr;

	PxRigidActor* actor = nullptr;
	PxShape* shape = nullptr;

	// 메시는 주로 정적 객체로 사용
	actor = m_physics->createRigidStatic(PxTransform(position));
	actor->setName("TriangleMesh");
	shape = PxRigidActorExt::createExclusiveShape(*actor,
		PxTriangleMeshGeometry(triangleMesh), *m_defaultMaterial);

	if (actor && shape) {
		shape->setSimulationFilterData(CreateFilterData(group, mask));
		shape->setQueryFilterData(CreateFilterData(group, mask));
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

std::shared_ptr<PhysicsObject> PhysicsEngine::CreateCapsuleController(
	const PxVec3& position, float radius, float height, 
	CollisionGroup group, CollisionGroup mask)
{
	if (!m_controllerManager) {
		Logger::Instance().Error("컨트롤러 매니저가 초기화되지 않음");
		return nullptr;
	}

	// 캡슐 컨트롤러 속성 설정
	PxCapsuleControllerDesc desc;
	desc.position = PxExtendedVec3(position.x, position.y, position.z);
	desc.radius = radius;
	desc.height = height;
	desc.stepOffset = 0.5f;          // 계단 등반 높이
	desc.slopeLimit = 0.707f;        // 약 45도
	desc.material = m_defaultMaterial;
	desc.upDirection = PxVec3(0, 1, 0);
	desc.reportCallback = m_controllerHitReport.get();    // 충돌 콜백
	desc.behaviorCallback = nullptr;  // 행동 콜백
	desc.contactOffset = 0.1f;        // 접촉 여유 거리

	// 컨트롤러 생성
	PxController* controller = m_controllerManager->createController(desc);
	if (!controller) {
		Logger::Instance().Error("캡슐 컨트롤러 생성 실패");
		return nullptr;
	}

	// 필터 데이터 설정
	PxRigidDynamic* actor = controller->getActor();
	PxShape* shape;
	actor->getShapes(&shape, 1);

	// 시뮬레이션 필터 데이터 설정
	shape->setSimulationFilterData(CreateFilterData(group, mask));

	// 쿼리 필터 데이터도 동일하게 설정 (레이캐스트 등에 사용)
	shape->setQueryFilterData(CreateFilterData(group, mask));

	// 운동학적 액터로 설정
	actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	// PhysicsObject 생성 및 반환
	auto physicsObject = std::make_shared<PhysicsObject>(actor);
	m_physicsObjects.push_back(physicsObject);

	Logger::Instance().Debug("캡슐 컨트롤러 생성됨. 위치: ({}, {}, {}), 반지름: {}, 높이: {}",
		position.x, position.y, position.z, radius, height);

	return physicsObject;
}


PxObstacleHandle PhysicsEngine::AddObstacle(
	const PxVec3& position, const PxVec3& dimensions, const PxQuat& rotation)
{
	if (!m_obstacleContext) {
		Logger::Instance().Error("장애물 컨텍스트가 초기화되지 않음");
		return PxObstacleHandle(0);
	}

	auto obstacle = std::make_unique<PxBoxObstacle>();
	obstacle->mPos = PxExtendedVec3(position.x, position.y, position.z);
	obstacle->mHalfExtents = dimensions * 0.5f;
	obstacle->mRot = PxQuat(rotation);

	PxObstacleHandle handle = m_obstacleContext->addObstacle(*obstacle);
	if (handle != PxObstacleHandle(0)) {
		m_obstacles.emplace_back(handle, std::move(obstacle));

		Logger::Instance().Debug("장애물 추가됨. 핸들: {}, 위치: ({}, {}, {}), 크기: ({}, {}, {})",
			handle,
			position.x, position.y, position.z,
			dimensions.x, dimensions.y, dimensions.z);
	}

	return handle;
}

void PhysicsEngine::RemoveObstacle(PxObstacleHandle handle)
{
	if (!m_obstacleContext) return;

	auto it = std::find_if(m_obstacles.begin(), m_obstacles.end(),
		[handle](const auto& pair) { return pair.first == handle; });

	if (it != m_obstacles.end()) {
		m_obstacleContext->removeObstacle(handle);
		m_obstacles.erase(it);
		Logger::Instance().Debug("장애물 제거됨. 핸들: {}", handle);
	}
}

void PhysicsEngine::ClearObstacles()
{
	if (!m_obstacleContext) return;

	for (const auto& [handle, obstacle] : m_obstacles) {
		m_obstacleContext->removeObstacle(handle);
	}
	m_obstacles.clear();
	Logger::Instance().Debug("모든 장애물 제거됨");
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
	// 트리거 처리
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}

	// 충돌 마스크 확인
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1)) {
		// CCT 필터링 - CCT는 운동학적 액터로 취급됨
		bool isKinematic0 = PxFilterObjectIsKinematic(attributes0);
		bool isKinematic1 = PxFilterObjectIsKinematic(attributes1);

		// CCT와 관련된 충돌인 경우
		if (isKinematic0 || isKinematic1) {
			pairFlags = PxPairFlag::eMODIFY_CONTACTS
				| PxPairFlag::eDETECT_CCD_CONTACT  // CCD 활성화
				| PxPairFlag::eNOTIFY_TOUCH_CCD    // CCD 접촉 알림
				| PxPairFlag::eNOTIFY_TOUCH_FOUND  // 접촉 시작 알림
				| PxPairFlag::eNOTIFY_TOUCH_LOST   // 접촉 종료 알림
				| PxPairFlag::eNOTIFY_CONTACT_POINTS; // 접촉점 알림
		}
		else {
			pairFlags = PxPairFlag::eCONTACT_DEFAULT
				| PxPairFlag::eNOTIFY_TOUCH_FOUND
				| PxPairFlag::eNOTIFY_TOUCH_LOST;
		}

		return PxFilterFlag::eDEFAULT;
	}

	return PxFilterFlag::eSUPPRESS;
}
