#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
namespace al { class SensorMsg; class ScreenPointer; class ScreenPointTarget; }
class DrcTouchPointer;
struct TouchCarryItemStateParam {
    TouchCarryItemStateParam();
    TouchCarryItemStateParam(float, float, float, float, float, float);
    float throwSpeed;
    float throwLift;
    float releaseLift;
    float holdOffset;
    float throwThreshold;
    float releaseOffset;
};
class TouchCarryItemState : public al::ActorStateBase {
public:
    TouchCarryItemState(al::LiveActor*, const TouchCarryItemStateParam*);
    void appear() override;
    void kill() override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*);
    void exeHold();
    void exeRelease();
    void setFinalPos();
    void exeThrow();
    bool isItemThrow() const;
    const sead::Vector3f& getThrowVelocity() const { return mThrowVelocity; }
    const sead::Vector3f& getReleaseVelocity() const { return mReleaseVelocity; }
private:
    const DrcTouchPointer* mPointer = nullptr;
    sead::Vector3f mThrowVelocity = sead::Vector3f::zero;
    sead::Vector3f mReleaseVelocity = sead::Vector3f::zero;
    const TouchCarryItemStateParam* mParam;
};
