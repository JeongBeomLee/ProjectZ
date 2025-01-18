#include "pch.h"
#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"
#include "InputManager.h"
#include "Logger.h"

Camera::Camera()
    : m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
    , m_projectionType(ProjectionType::Perspective)
    , m_fov(XM_PIDIV4)
    , m_aspectRatio(16.0f / 9.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_orthoWidth(1280.0f)
    , m_orthoHeight(720.0f)
    , m_isDirty(true)
{
    m_updatePriority = UpdatePriority::Camera;
    Logger::Instance().Debug("Camera 컴포넌트 생성됨");
}

void Camera::Initialize() 
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::Update(float deltaTime) 
{
    if (m_controlEnabled) {
        ProcessInput(deltaTime);
    }

    auto transform = GetGameObject()->GetTransform();

    if (transform->IsDirty() || m_isDirty) {
        UpdateViewMatrix();

        if (m_isDirty) {
            UpdateProjectionMatrix();
            m_isDirty = false;
        }

        UpdateFrustumPlanes();
    }
}

void Camera::Destroy()
{
	Logger::Instance().Debug("Camera 컴포넌트 제거됨");
}

void Camera::UpdateViewMatrix() 
{
    auto transform = GetGameObject()->GetTransform();
    const XMMATRIX& worldMatrix = transform->GetWorldMatrix();

    // 월드 행렬에서 카메라의 위치와 방향 추출
    XMVECTOR determinant;
    m_viewMatrix = XMMatrixInverse(&determinant, worldMatrix);
}

void Camera::UpdateProjectionMatrix() 
{
    if (m_projectionType == ProjectionType::Perspective) {
        // 원근 투영 행렬 생성
        m_projectionMatrix = XMMatrixPerspectiveFovLH(
            m_fov,              // 시야각
            m_aspectRatio,      // 종횡비
            m_nearPlane,        // 근평면
            m_farPlane         // 원평면
        );
    }
    else {
        // 직교 투영 행렬 생성
        m_projectionMatrix = XMMatrixOrthographicLH(
            m_orthoWidth,       // 뷰 너비
            m_orthoHeight,      // 뷰 높이
            m_nearPlane,        // 근평면
            m_farPlane         // 원평면
        );
    }

    //Logger::Instance().Debug("카메라 투영 행렬이 업데이트됨. 타입: {}",
    //    m_projectionType == ProjectionType::Perspective ? "원근" : "직교");
}

void Camera::UpdateFrustumPlanes() 
{
    // 뷰-투영 행렬 계산
    XMMATRIX viewProj = m_viewMatrix * m_projectionMatrix;

    // 행렬의 각 행
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, viewProj);

    // 왼쪽 평면
    m_frustumPlanes[0] = XMPlaneNormalize(XMVectorSet(
        m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41));

    // 오른쪽 평면
    m_frustumPlanes[1] = XMPlaneNormalize(XMVectorSet(
        m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41));

    // 아래쪽 평면
    m_frustumPlanes[2] = XMPlaneNormalize(XMVectorSet(
        m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42));

    // 위쪽 평면
    m_frustumPlanes[3] = XMPlaneNormalize(XMVectorSet(
        m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42));

    // 근평면
    m_frustumPlanes[4] = XMPlaneNormalize(XMVectorSet(
        m._14 + m._13, m._24 + m._23, m._34 + m._33, m._44 + m._43));

    // 원평면
    m_frustumPlanes[5] = XMPlaneNormalize(XMVectorSet(
        m._14 - m._13, m._24 - m._23, m._34 - m._33, m._44 - m._43));
}

bool Camera::IsInFrustum(const XMFLOAT3& point, float radius) const 
{
    // 바운딩 스피어의 중심을 각 평면에 대해 테스트
    XMVECTOR p = XMLoadFloat3(&point);

    for (int i = 0; i < 6; i++) {
        // 평면과 점 사이의 거리 계산
        float distance = XMVectorGetX(XMPlaneDotCoord(m_frustumPlanes[i], p));

        // 구가 평면의 뒤쪽에 완전히 있다면 보이지 않음
        if (distance < -radius) {
            return false;
        }
    }

    return true;
}

void Camera::ProcessInput(float deltaTime)
{
    if (!m_controlEnabled) return;

    auto& input = InputManager::Instance();
    auto transform = GetGameObject()->GetTransform();

    // 현재 회전값 가져오기
    auto rotation = transform->GetRotation();

    // 마우스 입력으로 회전 처리
    if (input.IsMouseButtonDown(1)) {  // 우클릭 중일 때
        if (!input.IsRelativeMouseMode()) {
            input.SetMouseMode(true);  // 상대 마우스 모드 활성화
            ShowCursor(FALSE);
        }

        POINT mouseDelta = input.GetMouseDelta();
        rotation.x += mouseDelta.y * m_rotateSpeed;
        rotation.y += mouseDelta.x * m_rotateSpeed;

        // 상하 회전 제한 (-89도 ~ 89도)
        rotation.x = std::max(-89.0f, std::min(89.0f, rotation.x));

        transform->SetRotation(rotation);
    }
    else if (input.IsRelativeMouseMode()) {
        input.SetMouseMode(false);  // 상대 마우스 모드 비활성화
		ShowCursor(TRUE);
    }

    // 키보드 입력으로 이동 처리
    float moveDistance = m_moveSpeed * deltaTime;
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(rotation.x),
        XMConvertToRadians(rotation.y),
        XMConvertToRadians(rotation.z));

    auto position = transform->GetPosition();
    XMVECTOR posVector = XMLoadFloat3(&position);

    // 전후좌우 이동
    if (input.IsKeyDown('W')) {  // 전진
        XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
        posVector += forward * moveDistance;
    }
    if (input.IsKeyDown('S')) {  // 후진
        XMVECTOR forward = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
        posVector -= forward * moveDistance;
    }
    if (input.IsKeyDown('A')) {  // 좌측 이동
        XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix);
        posVector -= right * moveDistance;
    }
    if (input.IsKeyDown('D')) {  // 우측 이동
        XMVECTOR right = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotationMatrix);
        posVector += right * moveDistance;
    }

    // 상하 이동
    if (input.IsKeyDown('E')) {  // 상승
        posVector += XMVectorSet(0.0f, moveDistance, 0.0f, 0.0f);
    }
    if (input.IsKeyDown('Q')) {  // 하강
        posVector -= XMVectorSet(0.0f, moveDistance, 0.0f, 0.0f);
    }

    XMStoreFloat3(&position, posVector);
    transform->SetPosition(position);
}

void Camera::MoveForward(float distance)
{
    auto transform = GetGameObject()->GetTransform();
    auto forward = transform->GetForward();
    auto position = transform->GetPosition();

    position.x += forward.x * distance * m_moveSpeed;
    position.y += forward.y * distance * m_moveSpeed;
    position.z += forward.z * distance * m_moveSpeed;

    transform->SetPosition(position);
}

void Camera::MoveRight(float distance)
{
    auto transform = GetGameObject()->GetTransform();
    auto right = transform->GetRight();
    auto position = transform->GetPosition();

    position.x += right.x * distance * m_moveSpeed;
    position.y += right.y * distance * m_moveSpeed;
    position.z += right.z * distance * m_moveSpeed;

    transform->SetPosition(position);
}

void Camera::MoveUp(float distance)
{
    auto transform = GetGameObject()->GetTransform();
    auto up = transform->GetUp();
    auto position = transform->GetPosition();

    position.x += up.x * distance * m_moveSpeed;
    position.y += up.y * distance * m_moveSpeed;
    position.z += up.z * distance * m_moveSpeed;

    transform->SetPosition(position);
}

void Camera::Rotate(float pitch, float yaw, float roll) 
{
    auto transform = GetGameObject()->GetTransform();
    auto rotation = transform->GetRotation();

    rotation.x += pitch * m_rotateSpeed;
    rotation.y += yaw * m_rotateSpeed;
	rotation.z += roll * m_rotateSpeed;

    // 피치 각도 제한 (-89도 ~ 89도)
    rotation.x = std::max(-89.0f, std::min(89.0f, rotation.x));

    transform->SetRotation(rotation);
}
