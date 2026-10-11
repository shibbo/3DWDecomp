#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class FlashingCtrl;
class RumbleCalculator;
}  // namespace al
class ActorStateSupportStroke;
class ItemStatePlayerHold;
class ItemStatePopUpFront;

/// Key that the player carries to a DoorLock and throws into it to open the door.
class DoorKey : public al::LiveActor {
public:
    explicit DoorKey(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    bool hideActor() override;
    void respawn() override;
    void control() override;
    void updateCollider() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPointSM(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) override;

    void triggerKillForce(bool isPlaySe);
    void killForce();
    void throwKey(al::HitSensor* pSensor);
    void updateOpenThrowPose(s32 step);
    void appearPopUpAbove();
    void appearPopUpFront();
    bool isAppearNext() const;
    bool tryKillBySandwichWall();
    void releaseKey();
    bool isInSameIsland() const;

    void exeWait();
    void exeWaitHide();
    void exePlayerHold();
    void exeThrow();
    void exeOpenThrow();
    void exePopUpAppear();
    void exeDisAppear();
    void exeStroked();

    /** @brief Marks the key as used by its door: it is killed instead of respawning. */
    void startUnlock() {
        mIsUsed = true;
        mIsEnableDisappearReaction = false;
    }

private:
    bool mIsUsed = false;
    bool mIsEnableDisappearReaction = true;
    al::HitSensor* mHolderSensor = nullptr;
    ItemStatePlayerHold* mStatePlayerHold = nullptr;
    ItemStatePopUpFront* mStateThrow = nullptr;
    ItemStatePopUpFront* mStatePullOut = nullptr;
    s32 mZoneId = 0;
    f32 mColliderOffsetY = 0.0f;
    f32 mColliderRadius = 0.0f;
    al::FlashingCtrl* mFlashingCtrl = nullptr;
    bool mIsBlinkHigh = false;
    bool mIsOpened = false;
    sead::Vector3f mOpenStartTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mOpenGoalTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mOpenStartFront = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mOpenGoalFront = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mInitTrans;
    sead::Vector3f mInitFront;
    f32 _1cc = 0.0f;
    f32 _1d0 = 0.0f;
    bool mIsSingleMode = false;
    bool _1d5;
    bool mIsShowGuide = false;
    bool mIsPlayerCanCarry = false;
    s32 mJumpPanelTimer = 0;
    s32 mSandwichTime;
    bool mIsFirstWait;
    bool mIsSameZone = true;
    bool mIsResetTrans = false;
    ActorStateSupportStroke* mStateStroke = nullptr;
    al::RumbleCalculator* mRumble = nullptr;
};

static_assert(sizeof(DoorKey) == 0x1f8);
