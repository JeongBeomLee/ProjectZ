#include "pch.h"
#include "ContactReportCallback.h"

void ContactReportCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
    for (PxU32 i = 0; i < nbPairs; i++) {
        const PxContactPair& cp = pairs[i];

        // �浹 �̺�Ʈ Ÿ�� Ȯ��
        if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
            // �浹 ����
            Logger::Instance().Info("Collision started between actors {} and {}",
                pairHeader.actors[0]->getName(),
                pairHeader.actors[1]->getName());

            // ������ ���� ����
            PxContactPairPoint contacts[10];
            PxU32 nbContacts = cp.extractContacts(contacts, 10);

            for (PxU32 j = 0; j < nbContacts; j++) {
                const PxVec3& position = contacts[j].position;
                const PxVec3& normal = contacts[j].normal;
                const PxVec3& impulse = contacts[j].impulse;

                Logger::Instance().Debug("Contact point {}: pos({}, {}, {}), normal({}, {}, {}), impulse: ({}, {}, {})",
                    j, position.x, position.y, position.z,
                    normal.x, normal.y, normal.z,
					impulse.x, impulse.y, impulse.z);
            }
        }
        else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
            // �浹 ����
            Logger::Instance().Info("Collision ended between actors {} and {}",
                pairHeader.actors[0]->getName(),
                pairHeader.actors[1]->getName());
        }
    }
}
