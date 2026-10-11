#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class HitSensor;
}
class KuriboTowerNode;

class KuriboTower : public al::LiveActor {
public:
    static s32 calcAppearDoubleMarioNum(const al::ActorInitInfo& rInfo);
    explicit KuriboTower(const char* pName);

    s32 getChildNum() const;
    KuriboTowerNode* getBottom() const;
    bool isHipDropping() const;
    void requestAttackReaction(al::HitSensor* pSensor);
    void startHipDrop(al::LiveActor* pAttacker);
    void endSleep();
    void releaseChild(KuriboTowerNode* pNode);
    bool isContact(const KuriboTowerNode* pNode) const;
    bool isContactTop(const KuriboTowerNode* pNode) const;
    bool isTopChildNode(const KuriboTowerNode* pNode) const;
    bool isBottom(const KuriboTowerNode* pNode) const;
    f32 getHipDropDownOffset(const KuriboTowerNode* pNode) const;
    void revieseVelocityIfExistObstacleForward(KuriboTowerNode* pNode);

    /**
     * @brief Checks whether the tower starts asleep.
     * @return Whether the bottom Goomba sleeps until it is woken up.
     */
    bool isStartSleep() const { return mIsStartSleep; }

    /**
     * @brief Checks whether the tower carries a Goomba variant used by the Bros. stages.
     * @return Whether the bottom Goomba uses the "RunBrosOnTower" run action.
     */
    bool isBrosOnTower() const { return mIsBrosOnTower; }

private:
    u8 _144[0x30];
    bool mIsStartSleep;
    u8 _175[0x1c];
    bool mIsBrosOnTower;
    u8 _192[0x26];
};
static_assert(sizeof(KuriboTower) == 0x1b8);
