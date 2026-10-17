#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
namespace al { class SensorMsg; class ScreenPointer; class ScreenPointTarget; class HitSensor; class AnimScaleController; }
struct ActorStateSupportFreezeParam {
    ActorStateSupportFreezeParam();
    ActorStateSupportFreezeParam(bool, int);
    ActorStateSupportFreezeParam(bool, int, bool, bool, int, const sead::Vector3f&);
    bool mEnableStroke;
    int mStrokeEffectInterval;
    bool mFreezeSubActors;
    bool mEmitItem;
    int mItemStrokeCount;
    sead::Vector3f mItemOffset;
};
static_assert(sizeof(ActorStateSupportFreezeParam) == 0x1c);
class ActorStateSupportFreeze : public al::ActorStateBase {
public:
    ActorStateSupportFreeze(al::LiveActor*, const ActorStateSupportFreezeParam*);
    void appear() override;
    void kill() override;
    void setScaleAnimTypeHard();
    int getStrokeFrame() const;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*);
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*);
    bool setTouchActor(al::ScreenPointer*);
    bool setTouchActor(al::HitSensor*);
    void exeBind();
private:
    al::LiveActor* mTouchActor = nullptr;
    al::AnimScaleController* mScaleController = nullptr;
    int mReleaseTimer = 0;
    float mAnimFrameRate = 1.0f;
    sead::Vector3f mVelocity = {0.0f, 0.0f, 0.0f};
    int mStrokeIdleTimer = 0;
    int mStrokeFrame = 0;
    bool mKeepFrozen = false;
    bool mItemEmitted = false;
    bool mAnimateScale = true;
    const ActorStateSupportFreezeParam* mParam;
};
static_assert(sizeof(ActorStateSupportFreeze) == 0x58);
