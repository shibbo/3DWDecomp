#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
#include <math/seadQuat.h>

namespace al { class EffectMtxSetter; class MtxConnector; }
class AssistSlideMapPartsGroup;
class GuideBalloon;
class AssistSlideMapParts : public al::LiveActor {
public:
    explicit AssistSlideMapParts(const char*);
    ~AssistSlideMapParts() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startClipped() override;
    void control() override;
    void setGroupHost(AssistSlideMapPartsGroup*);
    void touchAssist();
    bool isTypeOnOff() const;
    bool isTypeTimerOff() const;
    s32 calcMoveTime() const;
    void exeOffWait();
    void exeOn();
    void exeOnWait();
    void exeOffSign();
    void exeOff();
private:
    AssistSlideMapPartsGroup* mGroupHost = nullptr;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    al::EffectMtxSetter* mEffectMtxSetter = nullptr;
    al::MtxConnector* mConnector = nullptr;
    GuideBalloon* mGuideBalloon = nullptr;
    sead::Quatf mInitialQuat = sead::Quatf::unit;
    sead::Vector3f mInitialTrans = sead::Vector3f::zero;
    s32 mMoveAxis = 0;
    float mMoveDistance = 400.0f;
    float mMoveRate = 0.0f;
    float mMoveSpeed = 20.0f;
    s32 mMoveTime = -1;
    s32 mOnWaitTime = 300;
    s32 mMoveType = 0;
    s32 mGuideBalloonType = 0;
    s32 mFloorTouchFrames = 0;
};
static_assert(sizeof(AssistSlideMapParts) == 0x1d8);
