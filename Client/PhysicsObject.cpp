#include "pch.h"
#include "PhysicsObject.h"

PhysicsObject::PhysicsObject(PxRigidActor* actor) 
	: m_actor(actor) 
{
}

XMMATRIX PhysicsObject::GetTransformMatrix() const
{
    PxTransform transform = m_actor->getGlobalPose();

    // Quaternion을 행렬로 변환
    PxMat44 pxMatrix(transform);

    // PxMat44를 XMMATRIX로 변환
    return XMMATRIX(
        pxMatrix.column0.x, pxMatrix.column0.y, pxMatrix.column0.z, pxMatrix.column0.w,
        pxMatrix.column1.x, pxMatrix.column1.y, pxMatrix.column1.z, pxMatrix.column1.w,
        pxMatrix.column2.x, pxMatrix.column2.y, pxMatrix.column2.z, pxMatrix.column2.w,
        pxMatrix.column3.x, pxMatrix.column3.y, pxMatrix.column3.z, pxMatrix.column3.w
    );
}
