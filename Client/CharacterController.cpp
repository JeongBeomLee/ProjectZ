// CharacterController.cpp
#include "pch.h"
#include "CharacterController.h"
#include "GameObject.h"
#include "Transform.h"
#include "Engine.h"
#include "InputManager.h"
#include "Logger.h"
#include "PhysicsEngine.h"
#include "PhysicsObject.h"

CharacterController::CharacterController()
{
    m_updatePriority = UpdatePriority::Physics;
}

CharacterController::~CharacterController()
{
    Destroy();
}

void CharacterController::Initialize()
{
    // 캡슐 컨트롤러 생성
    auto transform = GetGameObject()->GetTransform();
    auto position = transform->GetPosition();

    auto physicsObject = Engine::Instance().GetPhysicsEngine()->CreateCapsuleController(
        PxVec3(position.x, position.y, position.z),
        0.5f,   // 반지름
        2.0f,   // 높이
        CollisionGroup::Character,
        CollisionGroup::Default | CollisionGroup::Ground |
        CollisionGroup::Obstacle | CollisionGroup::Character
    );

    if (!physicsObject) {
        Logger::Instance().Error("캐릭터 컨트롤러 생성 실패");
        return;
    }

	m_controller = Engine::Instance().GetPhysicsEngine()->GetControllerManager()->getController(0);
    Logger::Instance().Debug("캐릭터 컨트롤러 초기화됨");
}

void CharacterController::Update(float deltaTime)
{
    if (!m_controller || !IsEnabled()) return;

    HandleInput(deltaTime);
    UpdateTransform();
}

void CharacterController::Destroy()
{
    if (m_controller) {
        m_controller->release();
        m_controller = nullptr;
    }
}

void CharacterController::Move(const XMFLOAT3& displacement, float minDist, float deltaTime)
{
	auto obstacleContext = Engine::Instance().GetPhysicsEngine()->GetObstacleContext();
    if (!m_controller) return;
	if (!obstacleContext) return;

    PxVec3 disp(displacement.x, displacement.y, displacement.z);

    // 캐릭터 이동 필터 설정
    PxControllerFilters filters;
    PxFilterData filterData(
        static_cast<PxU32>(CollisionGroup::Character),
        static_cast<PxU32>(CollisionGroup::Default | CollisionGroup::Ground | CollisionGroup::Obstacle),
        0, 0);
    filters.mFilterData = &filterData;


    // 이동 수행
    PxControllerCollisionFlags collisionFlags =
        m_controller->move(disp, minDist, deltaTime, filters, obstacleContext);

    // 지면 접촉 상태 갱신 - 비트 연산으로 수정
    m_isOnGround = (collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN));
}

void CharacterController::Jump(float jumpForce)
{
    if (!m_controller || !m_isOnGround) return;

    m_velocity.y = jumpForce;
    m_isOnGround = false;
}

const PxExtendedVec3& CharacterController::GetPosition() const
{
    static PxExtendedVec3 defaultPos(0.0f);
    return m_controller ? m_controller->getPosition() : defaultPos;
}

void CharacterController::SetStepOffset(float offset)
{
    if (m_controller) {
        m_controller->setStepOffset(offset);
    }
}

void CharacterController::SetSlopeLimit(float slopeLimit)
{
    if (m_controller) {
        m_controller->setSlopeLimit(cosf(slopeLimit));
    }
}

void CharacterController::SetContactOffset(float offset)
{
    if (m_controller) {
        m_controller->setContactOffset(offset);
    }
}

void CharacterController::UpdateTransform()
{
    if (!m_controller) return;

    // 컨트롤러의 위치를 Transform 컴포넌트에 반영
    const PxExtendedVec3& position = m_controller->getPosition();

    auto transform = GetGameObject()->GetTransform();
    transform->SetPosition(XMFLOAT3(
        static_cast<float>(position.x),
        static_cast<float>(position.y),
        static_cast<float>(position.z)
    ));
}

void CharacterController::HandleInput(float deltaTime)
{
    auto& input = InputManager::Instance();
    XMFLOAT3 moveDir(0.0f, 0.0f, 0.0f);

    // 키보드 입력에 따른 이동 방향 결정
    if (input.IsKeyDown('W')) moveDir.z += 1.0f;
    if (input.IsKeyDown('S')) moveDir.z -= 1.0f;
    if (input.IsKeyDown('A')) moveDir.x -= 1.0f;
    if (input.IsKeyDown('D')) moveDir.x += 1.0f;

    // 스페이스바로 점프
    if (input.IsKeyPressed(VK_SPACE) && m_isOnGround) {
        Jump(m_jumpForce);
    }

    // 중력 적용
    if (!m_isOnGround) {
        m_velocity.y += m_gravity * deltaTime;
    }

    // 이동 벡터 정규화 및 속도 적용
    if (abs(moveDir.x) > 0.0f || abs(moveDir.z) > 0.0f) {
        float length = sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        moveDir.x = moveDir.x / length * m_moveSpeed * deltaTime;
        moveDir.z = moveDir.z / length * m_moveSpeed * deltaTime;
    }

    // Y축 속도 적용
    moveDir.y = m_velocity.y * deltaTime;

    // 이동 적용
    Move(moveDir, 0.001f, deltaTime);
}