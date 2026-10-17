#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
class Imozo;
namespace al { class SensorMsg; class HitSensor; }

class ImozoStateBlockRailMove : public al::HostStateBase<Imozo> {
public:
    ImozoStateBlockRailMove(const char* pName, Imozo* pHost);
    bool isEnableReactionMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    bool tryStartReaction(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf);
    void appear() override;
    void control() override;
    void exeMove();
    void exeFall();
    void exeLand();
    void exeReaction();
    void exeTurnJump();
    void turnJumpForce();
    bool isEnableSupportFreeze() const;
private:
    f32 mRailAccel = 0.7f;
    sead::Vector3f mJumpStart;
    s32 mReactionTimer = 600;
    s32 mBoomerangTimer = 600;
};
static_assert(sizeof(ImozoStateBlockRailMove) == 0x38);
