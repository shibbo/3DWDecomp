#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BlockRailPartsGroup; class BlockRailRider; }
class LiftMikeBlockRail : public al::LiveActor {
public:
    explicit LiftMikeBlockRail(const char*);
    ~LiftMikeBlockRail() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void exeWait();
    void exeMove();
    void exeVibration();
    void exeHold();
    void exeBack();
private:
    al::BlockRailPartsGroup* mRailGroup = nullptr;
    al::BlockRailRider* mRailRider = nullptr;
    sead::Matrix34f mVibrationMtx = sead::Matrix34f::ident;
    float mVibrationAngle = 0.0f;
    float mVibrationStrength = 0.0f;
    int mGuideBalloonType = 0;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    float mSpeed = 0.0f;
    float mPropellerAngle = 0.0f;
    int mVibrationTimer = 0;
};
static_assert(sizeof(LiftMikeBlockRail) == 0x1b0);
