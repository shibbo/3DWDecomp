#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
class AnimScaleController;
class HitSensor;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
}

struct ActorStateSupportFreezeParam {
    ActorStateSupportFreezeParam();
    ActorStateSupportFreezeParam(bool isStroke, int strokeEffectInterval);
    ActorStateSupportFreezeParam(bool isStroke, int strokeEffectInterval, bool isSyncSubActor,
                                 bool isAppearItem, int strokeFrame,
                                 const sead::Vector3f& rItemOffset);

    bool mIsStroke;
    int mStrokeEffectInterval;
    bool mIsSyncSubActor;
    bool mIsAppearItem;
    int mStrokeFrame;
    sead::Vector3f mItemOffset;
};

class ActorStateSupportFreeze : public al::ActorStateBase {
public:
    ActorStateSupportFreeze(al::LiveActor* pHost, const ActorStateSupportFreezeParam* pParam);
    /** @brief Destroys the support-freeze state. */
    ~ActorStateSupportFreeze() override = default;
    void appear() override;
    void kill() override;
    void setScaleAnimTypeHard();
    int getStrokeFrame() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSensor);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    bool setTouchActor(al::ScreenPointer* pPointer);
    bool setTouchActor(al::HitSensor* pSensor);
    void exeBind();

    /** @brief Allows another stroke item to appear after the host is reused. */
    void resetAppearItem() { mIsItemAppeared = false; }

    /** @brief Gets the actor that is currently freezing the host. */
    al::LiveActor* getTouchActor() const { return mTouchActor; }

    /** @brief Directly sets the actor freezing the host, e.g. one relayed from a linked actor. */
    void forceSetTouchActor(al::LiveActor* pActor) { mTouchActor = pActor; }

    /** @brief Keeps the host frozen until the state is ended from outside. */
    void onKeepFreeze() { mIsKeepFreeze = true; }

private:
    al::LiveActor* mTouchActor = nullptr;
    al::AnimScaleController* mScaleController = nullptr;
    int mStepAfterTouch = 0;
    float mSklAnimFrameRate = 1.0f;
    sead::Vector3f mVelocity = {0.0f, 0.0f, 0.0f};
    int mNoStrokeFrame = 0;
    int mStrokeFrame = 0;
    bool mIsKeepFreeze = false;
    bool mIsItemAppeared = false;
    bool mIsScaleAnim = true;
    const ActorStateSupportFreezeParam* mParam;
};

static_assert(sizeof(ActorStateSupportFreezeParam) == 0x1c);
static_assert(sizeof(ActorStateSupportFreeze) == 0x58);
