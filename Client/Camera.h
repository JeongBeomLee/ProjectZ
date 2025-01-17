#pragma once
#include "Component.h"

class Camera : public Component {
public:
    enum class ProjectionType {
        Perspective,
        Orthographic
    };

    Camera();
    ~Camera() override = default;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Destroy() override;

    // 행렬 업데이트
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    // 기본 행렬 접근자
    const XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }
    const XMMATRIX& GetProjectionMatrix() const { return m_projectionMatrix; }

    // 투영 타입 설정
    void SetProjectionType(ProjectionType type) {
        m_projectionType = type;
        m_isDirty = true;
    }

    // 원근 투영 속성
    void SetPerspectiveProperties(float fov, float aspectRatio, float nearPlane, float farPlane) {
        m_fov = fov;
        m_aspectRatio = aspectRatio;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_isDirty = true;
    }

    // 직교 투영 속성
    void SetOrthographicProperties(float width, float height, float nearPlane, float farPlane) {
        m_orthoWidth = width;
        m_orthoHeight = height;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_isDirty = true;
    }

    // 프러스텀 평면 업데이트 및 컬링 지원
    void UpdateFrustumPlanes();
    bool IsInFrustum(const XMFLOAT3& point, float radius) const;

    // 카메라 이동/회전 제어
    void ProcessInput(float deltaTime);
    void EnableControl(bool enable) { m_controlEnabled = enable; }
    bool IsControlEnabled() const { return m_controlEnabled; }

    // 카메라 이동/회전 속도 설정
    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
    void SetRotateSpeed(float speed) { m_rotateSpeed = speed; }

    // 테스트용 카메라 움직임 메서드
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void Rotate(float pitch, float yaw, float roll);

private:
    // 변환 행렬
    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;

    // 투영 타입
    ProjectionType m_projectionType;

    // 원근 투영 속성
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    // 직교 투영 속성
    float m_orthoWidth;
    float m_orthoHeight;

    // 프러스텀 평면 (컬링용)
    XMVECTOR m_frustumPlanes[6];

    bool m_isDirty;

    // 카메라 제어 관련 멤버 변수
    bool m_controlEnabled = false;
    float m_moveSpeed = 5.0f;
    float m_rotateSpeed = 0.1f;
};