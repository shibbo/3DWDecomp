#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ComboCounter;
}  // namespace al
class BombStateExplosion;
class ItemBubble;
class ItemStatePlayerHold;
class ItemStatePopUpFront;
class TouchCarryItemState;

/** @brief Carryable bomb item: counts down once lit and explodes, damaging everything around. */
class Bomb : public al::LiveActor {
public:
    explicit Bomb(const char* pName, bool isItem = false);
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void appear() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void kick(const sead::Vector3f& rDir, al::HitSensor* pKicker);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableJumpPanel() const;
    bool tryStartCountDown();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void control() override;
    void appearTrampled(const al::LiveActor* pTrampler, const al::SensorMsg* pMsg);
    void appearPopUpFront();
    bool isAttach() const;
    void setNerveExplosion();
    void setNerveExplosionOffAttackToPlayer();
    bool tryExplosionByAreaOrMaterialCode();
    bool updateCountDown(bool isAttackToPlayer);
    void updateCollider() override;
    void exeWait();
    void updateVelocity();
    void exeAttach();
    void exeFall();
    void exePlayerHold();
    void exeThrow();
    void exeExplosion();
    void exeTrampled();
    void exePopUpAppear();
    void exeKicked();
    void exeReaction();
    void exeDRCHold();
    void exeJump();

private:
    void kickHorizontal(sead::Vector3f* pDir, al::HitSensor* pKicker);

    ItemStatePlayerHold* mStatePlayerHold = nullptr;     // 0x148
    ItemStatePopUpFront* mStatePopUpAppear = nullptr;    // 0x150
    ItemStatePopUpFront* mStateThrow = nullptr;          // 0x158
    BombStateExplosion* mStateExplosion = nullptr;       // 0x160
    TouchCarryItemState* mStateTouchCarry = nullptr;     // 0x168
    al::ComboCounter* mComboCounter = nullptr;           // 0x170
    al::HitSensor* mHolderSensor = nullptr;              // 0x178
    al::LiveActor* mTouchActor = nullptr;                // 0x180
    s32 mUserId = -1;                                    // 0x188
    s32 mCountDown = -1;                                 // 0x18c
    bool mIsItem;                                        // 0x190
    f32 mColliderRadius = 50.0f;                         // 0x194
    bool mIsSingleMode = false;                          // 0x198
    bool mIsShowGuide = false;                           // 0x199
    bool mIsPlayerCanCarry = false;                      // 0x19a
};
static_assert(sizeof(Bomb) == 0x1a0);
