#pragma once
#include "Event.h"
#include "PhysicsTypes.h"

namespace Event
{
    // ���� �浹 �̺�Ʈ
    struct CollisionEvent : public IEvent {
		CollisionEvent() = default;
        CollisionEvent(const PxActor* actor1, const PxActor* actor2,
            const PxVec3& position, const PxVec3& normal, float impulse)
            : actor1(actor1)
            , actor2(actor2)
            , position(position)
            , normal(normal)
            , impulse(impulse) {}

        const PxActor* actor1;
        const PxActor* actor2;
        PxVec3 position;
        PxVec3 normal;
        float impulse;
    };

    // ���ҽ� �ε� �̺�Ʈ
    struct ResourceEvent : public IEvent {
        enum class Type {
            Started,
            Completed,
            Failed
        };
		ResourceEvent() = default;
        ResourceEvent(const std::string& path, Type type, const std::string& error = "")
            : path(path)
            , type(type)
            , error(error) {}

        std::string path;
        Type type;
        std::string error;
    };

    // �Է� �̺�Ʈ
    struct InputEvent : public IEvent {
        enum class Type {
            KeyDown,
            KeyUp,
            MouseMove,
            MouseButtonDown,
            MouseButtonUp
        };
		InputEvent() = default;
        InputEvent(Type type, int code, float x = 0.0f, float y = 0.0f)
            : type(type)
            , code(code)
            , x(x)
            , y(y) {}

        Type type;
        int code;
        float x, y;
    };
}