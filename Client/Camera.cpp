#include "pch.h"
#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"
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
    Logger::Instance().Debug("Camera 컴포넌트 생성됨");
}

void Camera::Initialize() {
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::Update(float deltaTime) {
    if (m_isDirty || GetGameObject()->GetTransform()->IsDirty()) {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
        UpdateFrustumPlanes();
        m_isDirty = false;
    }
}

void Camera::Destroy()
{
	Logger::Instance().Debug("Camera 컴포넌트 제거됨");
}

void Camera::UpdateViewMatrix() {
    auto transform = GetGameObject()->GetTransform();
    const XMMATRIX& worldMatrix = transform->GetWorldMatrix();

    // 월드 행렬에서 카메라의 위치와 방향 추출
    XMVECTOR determinant;
    m_viewMatrix = XMMatrixInverse(&determinant, worldMatrix);
}

void Camera::UpdateProjectionMatrix() {
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

    Logger::Instance().Debug("카메라 투영 행렬이 업데이트됨. 타입: {}",
        m_projectionType == ProjectionType::Perspective ? "원근" : "직교");
}

void Camera::UpdateFrustumPlanes() {
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

bool Camera::IsInFrustum(const XMFLOAT3& point, float radius) const {
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