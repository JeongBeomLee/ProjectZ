#include "pch.h"
#include "ContactReportCallback.h"
#include "Event.h"
#include "EventManager.h"

void ContactReportCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
    for (PxU32 i = 0; i < nbPairs; i++) {
        const PxContactPair& cp = pairs[i];

        // 충돌 이벤트 타입 확인
        if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
            // 충돌 시작
            Logger::Instance().Info("'{}' 와 '{}'가 충돌 발생.",
                pairHeader.actors[0]->getName(),
                pairHeader.actors[1]->getName());

            // 접촉점 정보 추출
            PxContactPairPoint contacts[1];
            PxU32 nbContacts = cp.extractContacts(contacts, 1);

			// 충돌 이벤트 발행
            if (nbContacts > 0) {
                Event::CollisionEvent event(
                    pairHeader.actors[0],
                    pairHeader.actors[1],
                    contacts[0].position,
                    contacts[0].normal,
                    contacts[0].impulse.magnitude()
                    );

                EventManager::Instance().Publish(event);
            }
        }
        else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
            // 충돌 종료
            Logger::Instance().Info("'{}' 와 '{}'가 충돌 종료.",
                pairHeader.actors[0]->getName(),
                pairHeader.actors[1]->getName());
        }
    }
}
