#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ActorRailBrakeMover;
/** @brief Floating ghost that fades when struck and is vulnerable to light. */
class Teren : public al::LiveActor {
public:
    explicit Teren(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void killBySwitch();
    void control() override;
    void appear() override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeAppear();
    void exeMove();
    void exeReaction();
    void exeReactionWait();
    void exeReactionAppear();
    void exeDamageLight();
    void exeDie();
private:
    ActorRailBrakeMover* mRailMover = nullptr;
    sead::Vector3f mInitialPos = sead::Vector3f(0.0f, 0.0f, 0.0f);
    s32 mUnused = 0;
    s32 mFloatStep = 0;
    s32 mLightTimer = 0;
};
static_assert(sizeof(Teren) == 0x168);
