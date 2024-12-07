#pragma once
#include "PhysX/include/PxPhysicsAPI.h"
using namespace physx;

enum class CollisionGroup : PxU32 {
    Default = (1 << 0),     // 기본 그룹
    Player = (1 << 1),      // 플레이어
    Enemy = (1 << 2),       // 적
    Projectile = (1 << 3),  // 발사체
    Ground = (1 << 4),      // 지형
    Trigger = (1 << 5)      // 트리거 영역
};

// 비트 연산자 오버로딩
inline CollisionGroup operator|(CollisionGroup a, CollisionGroup b) {
    return static_cast<CollisionGroup>(
        static_cast<PxU32>(a) | static_cast<PxU32>(b));
}

inline CollisionGroup operator&(CollisionGroup a, CollisionGroup b) {
    return static_cast<CollisionGroup>(
        static_cast<PxU32>(a) & static_cast<PxU32>(b));
}

inline CollisionGroup& operator|=(CollisionGroup& a, CollisionGroup b) {
    a = a | b;
    return a;
}

inline CollisionGroup& operator&=(CollisionGroup& a, CollisionGroup b) {
    a = a & b;
    return a;
}