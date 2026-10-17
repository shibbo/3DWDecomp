#pragma once
#include "Library/LiveActor/LiveActor.hpp"
/** @brief Walking platform enemy that tumbles under a player's weight. */
class Zuruzuru : public al::LiveActor {
public:
    Zuruzuru(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool isEnableTumble() const;
    void exeWait();
    void exeTumbleSign();
    void exeWalk();
    void exeTurn();
    void exeFind();
    void exeTumble();
    void exeTumbleWait();
    void exeStandUp();
private:
    sead::Quatf mInitialQuat = sead::Quatf::unit;
    sead::Vector3f mWalkStart;
    sead::Vector3f mDirection = sead::Vector3f::ez;
    f32 mWalkDistance = 0.0f;
    f32 mAxisThreshold = 200.0f;
    f32 mTurnAngle = 0.0f;
    s32 mStepsWithoutRider = 0;
    s32 mPlayerId = -1;
};
static_assert(sizeof(Zuruzuru) == 0x180);
