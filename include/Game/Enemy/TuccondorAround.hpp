#pragma once
#include "Enemy/TuccondorBase.hpp"
class TuccondorGlass;
class TuccondorGlassBlower;
class ActorStateSupportFreeze;
/** @brief Conkdor that tracks a target moving around a rail. */
class TuccondorAround : public TuccondorBase {
public:
    TuccondorAround(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeWait();
    void exeSupportFreeze();
    void exeAttack();
    void exeAttackWait();
    void exeAttackEnd();
    void exeAttackSign();
private:
    TuccondorGlass* mGlass = nullptr;
    TuccondorGlassBlower* mGlassBlower = nullptr;
    f32 mTargetSpeed = 5.0f;
    f32 mTargetOffset = 0.0f;
    s32 mAttackWaitTime = 60;
    sead::Vector3f mUnused;
    ActorStateSupportFreeze* mFreeze = nullptr;
};
static_assert(sizeof(TuccondorAround) == 0x188);
