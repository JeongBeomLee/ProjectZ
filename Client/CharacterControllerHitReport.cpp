#include "pch.h"
#include "CharacterControllerHitReport.h"
#include "PhysicsTypes.h"
#include "EventManager.h"
#include "Logger.h"

void CharacterControllerHitReport::onShapeHit(const PxControllerShapeHit& hit)
{
    // 일반 물리 객체와의 충돌
    Event::CollisionEvent event(
        hit.controller->getActor(),
        hit.shape->getActor(),
        PxVec3(static_cast<float>(hit.worldPos.x), 
            static_cast<float>(hit.worldPos.y), 
            static_cast<float>(hit.worldPos.z)),
        hit.worldNormal,
        hit.length
    );

    EventManager::Instance().Publish(event);

    Logger::Instance().Debug("캐릭터가 물리 객체와 충돌. 충돌 위치: ({}, {}, {})",
        hit.worldPos.x, hit.worldPos.y, hit.worldPos.z);
}

void CharacterControllerHitReport::onControllerHit(const PxControllersHit& hit)
{
    // 다른 캐릭터 컨트롤러와의 충돌
    Event::CollisionEvent event(
        hit.controller->getActor(),
        hit.other->getActor(),
        PxVec3(static_cast<float>(hit.worldPos.x),
            static_cast<float>(hit.worldPos.y),
            static_cast<float>(hit.worldPos.z)),
        PxVec3(0.0f),  // 컨트롤러 충돌에서는 normal이 제공되지 않음
        0.0f           // 충격량도 제공되지 않음
    );

    EventManager::Instance().Publish(event);

    Logger::Instance().Debug("캐릭터가 다른 캐릭터와 충돌. 충돌 위치: ({}, {}, {})",
        hit.worldPos.x, hit.worldPos.y, hit.worldPos.z);
}

void CharacterControllerHitReport::onObstacleHit(const PxControllerObstacleHit& hit)
{
    // 장애물과의 충돌
    //Event::CollisionEvent event(
    //    hit.controller->getActor(),
    //    nullptr,  // 장애물은 PxActor가 없음
    //    PxVec3(static_cast<float>(hit.worldPos.x),
    //        static_cast<float>(hit.worldPos.y),
    //        static_cast<float>(hit.worldPos.z)),
    //    hit.worldNormal,
    //    0.0f      // 충격량은 제공되지 않음
    //);

    //EventManager::Instance().Publish(event);

    Logger::Instance().Debug("캐릭터가 장애물과 충돌. 충돌 위치: ({}, {}, {})",
        hit.worldPos.x, hit.worldPos.y, hit.worldPos.z);
}
