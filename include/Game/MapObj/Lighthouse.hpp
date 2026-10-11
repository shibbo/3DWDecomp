#pragma once

#include <attributes.h>

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Camera/CameraLookAtPoint.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "MapObj/DisasterModeController.hpp"

namespace al {
class AreaObj;
class CameraPoserFixActor;
class FunctorBase;
class PlacementId;
}  // namespace al

class FlingPole;
class GoalItem;
class InkPatch;
class SinkedItem;

/**
 * @brief Plays a lighthouse's look-at camera along the points of a CameraLookAtPoint.
 */
class LookAtPointInterpole {
public:
    /**
     * @brief Capture the current camera as the start of the interpolation.
     */
    void captureCamera() {
        mStartAt = al::getCameraAt_RS(mActor, 0);
        mAt = mStartAt;
        mStartPos = al::getCameraPos_RS(mActor, 0);
        mPos = mStartPos;
    }

    /**
     * @brief Set the point list to follow.
     * @param pPoint The camera points.
     * @param moveStep Frames taken to move from the current camera to the first point.
     */
    void setup(CameraLookAtPoint* pPoint, s32 moveStep) {
        mLookAtPoint = pPoint;
        mStep = 0;
        mDelayStep = 0;
        mMoveStep = moveStep;
        mIsActive = false;
    }

    /**
     * @brief Start the camera from the current camera pose.
     */
    void start() {
        captureCamera();
        al::startCamera_RS(mActor, mTicket, -1);
        mPointIndex = 0;
        mStep = 1;
        mIsActive = true;
        mIsMoving = true;
        mIsWaitEnd = false;
        mIsPointWait = false;
        mIsLastPoint = false;
        mIsEnd = false;
    }

    /**
     * @brief End the camera.
     */
    void end() { al::endCamera_RS(mActor, mTicket, -1, false); }

    /**
     * @brief Move from the start camera to the first point.
     */
    void updateWait() {
        sead::Vector3f at = sead::Vector3f::zero;
        sead::Vector3f pos = sead::Vector3f::zero;
        s32 step = mStep - mDelayStep;

        if (step >= 0) {
            if (mStep < mMoveStep + mDelayStep) {
                f32 rate = mMoveStep != 0 ? static_cast<f32>(step) / mMoveStep : 1.0f;
                mLookAtPoint->interpolateInOut(mPointIndex, mStartAt, mStartPos, rate, &at, &pos);
            } else {
                if (mLookAtPoint->getPointCount() == 1) {
                    mIsPointWait = true;
                    mIsLastPoint = true;
                    mIsMoving = false;
                    mLastWaitStep = 0;
                }

                mLookAtPoint->interpolateInOut(mPointIndex, mStartAt, mStartPos, 1.0f, &at, &pos);
                mStep = -2;
                mIsWaitEnd = true;
            }

            mPos = pos;
            mAt = at;
        }

        mStep++;
    }

    /**
     * @brief Move between the camera points.
     */
    void updatePlaying() {
        u32 index = mPointIndex;
        s32 waitStep = mLookAtPoint->getWaitTime(index);
        s32 moveStep = mLookAtPoint->getMoveTime(index);
        sead::Vector3f at = sead::Vector3f::zero;
        sead::Vector3f pos = sead::Vector3f::zero;

        if (mStep == waitStep) {
            mIsPointWait = true;
            mIsMoving = false;
        }

        s32 step = mStep - waitStep;
        if (step >= 0) {
            if (moveStep != 0 && mStep < moveStep + waitStep) {
                mLookAtPoint->interpolate(index, index + 1, static_cast<f32>(step) / moveStep, &at,
                                          &pos);
                if (mIsPointWait && mStep > waitStep) {
                    mIsPointWait = false;
                }
            } else {
                if (moveStep == 0) {
                    mLookAtPoint->interpolate(index, index + 1, 1.0f, &mAt, &mPos);
                    index = mPointIndex;
                    mIsPointWait = false;
                }

                if (index + 2 < mLookAtPoint->getPointCount()) {
                    mPointIndex = index + 1;
                    mStep = 0;
                    al::resetClippingDistanceStates(mActor);
                    mStep++;
                    return;
                }

                mLastWaitStep = 0;
                mIsLastPoint = true;
                mLookAtPoint->interpolateIn(index + 1, mStartAt, mStartPos, 1.0f, &at, &pos);
            }

            mPos = pos;
            mAt = at;
        }

        if (mStep == 0) {
            al::resetClippingDistanceStates(mActor);
        }

        mStep++;
    }

    /**
     * @brief Update the camera.
     */
    void update() {
        if (!mIsActive) {
            return;
        }

        if (!mIsWaitEnd) {
            updateWait();
        } else if (!mIsLastPoint) {
            updatePlaying();
        } else {
            s32 waitStep = mLastWaitStep++;
            if (waitStep == mLookAtPoint->getWaitTime(mPointIndex)) {
                mTicket->getPoser()->setEndInterpoleStep(mLookAtPoint->getMoveTime(mPointIndex));
                mIsEnd = true;
                mIsPointWait = false;
                mIsActive = false;
                mIsMoving = false;
            }
        }
    }

    /**
     * @brief Check whether the camera reached the end of its points.
     * @return True once the last point was held.
     */
    bool isEnd() const { return mIsEnd; }

    al::LiveActor* mActor;
    al::CameraTicket* mTicket;
    CameraLookAtPoint* mLookAtPoint;
    u32 mPointIndex;
    s32 mStep;
    s32 mDelayStep;
    s32 mMoveStep;
    s32 mLastWaitStep;
    sead::Vector3f mPos;
    sead::Vector3f mAt;
    sead::Vector3f mStartPos;
    sead::Vector3f mStartAt;
    bool mIsActive;
    bool mIsMoving;
    bool mIsWaitEnd;
    bool mIsPointWait;
    bool mIsLastPoint;
    bool mIsEnd;
};

static_assert(sizeof(LookAtPointInterpole) == 0x68);

/**
 * @brief The lighthouse of a Bowser's Fury island, lit up by the island's Cat Shines.
 */
class Lighthouse : public al::LiveActor, public DisasterModeController::IUseEventReceiver {
public:
    /// Which Cat Shine effect the lighthouse shows.
    enum FlagState : s32 {
        FlagState_None = -1,
        FlagState_Ink = 0,
        FlagState_Partial = 1,
        FlagState_Complete = 2,
    };

    explicit Lighthouse(const char* pName);

    void disasterModeFadeInDoneFunc();
    void init(const al::ActorInitInfo& rInfo) override;
    bool initActiveScenario(s32 scenario);
    void activateScenarioAnim(bool isActivate);
    void initAfterPlacement() override;
    void setPhaseColor();
    void setScenarioButtons(al::LiveActor* pActor);
    void showMainLighthouse(bool isShow);
    void playEffect(FlagState state);
    void setupCameraLookAtPoint(CameraLookAtPoint* pPoint, s32 moveStep);
    void setLighthouseShone();
    void setInkPatchOrNext();
    bool handleInkPatch();
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void updateLinkedTrans(const sead::Vector3f& rTrans) override;
    void onDisasterModeStateChange(DisasterModeController::State state) override;
    void killInkPillar();
    void startInkPillar();
    bool isLod() const;
    virtual void updateLod();
    void handleFlagState();
    void killSinkedItem();
    bool jumpToFinish();
    void setBirdsAppeared();
    void exeWaitForGoalItemArrival();
    void exeLighthouseLight();
    void tryStartGlassBlinkAnim();
    void exeSkippedLighthouseLight();
    void exeDecideNormalState();
    void exeWaitBowserExit();
    void exeWaitGigaBellDemo();
    void exeSkipWaitBowserExit();
    void exeCameraShowInkPatch();
    void exeWaitPatchFadeOutForSpecialInkPatch();
    void exeCameraInkFadeOut();
    void exeCameraWaitSpecialInkPatch();
    void cleanUpWaitSpecialInkPatch();
    void exeWaitCameraSpecialInkPatchFadeOut();
    void exeCameraFlagUp();
    void cleanUpCameraFlag();
    void exeSkippedFlagUp();
    void exePhase0End();
    void exePhase0EndIdle();
    void exeCameraRail();
    void exeCameraReturn();
    void exeGuideMessage();
    void exeCameraOut();
    NOINLINE void setScenarioAnim();
    void exeCameraFinish();
    void exeWait();
    bool isCameraSequenceStarted();
    void setCurrentScenarioID(s32 scenarioId);
    void setInkMeNot(bool isInkMeNot);
    void setInkLinkID(s32 linkId);
    void setInkPatch(InkPatch* pInkPatch, s32 disappearStep, s32 waitStep);
    void setExitAngleSettings(bool isReturnSetDir, f32 angleH, f32 angleV, f32 dirRate,
                              f32 distanceRate, bool isReturnByStep, s32 returnStep,
                              f32 returnOffsetY, s32 returnEndStep);
    void setLandedPos(sead::Vector3f& rPos);
    void setReturnPos(sead::Vector3f& rPos, f32 distance, bool isKeep);
    void setOffsets(f32 distanceOffset, f32 heightOffset);
    void storeCamera();
    void startGoalItemArrivalCutscene(GoalItem* pGoalItem);
    void setSinkedItem(SinkedItem* pSinkedItem);
    bool isOkayToChangeFlag();
    s32 getFinshedTotalCount();
    void turnOnInk(s32 finishedCount);
    void setActiveScenario(s32 scenario);
    void setGoalItemArrived(bool isSkip);

    /**
     * @brief Block or unblock the end of the goal item cutscene.
     * @param isBlocked Whether the end is blocked.
     */
    void setFinishBlocked(bool isBlocked) { mIsFinishBlocked = isBlocked; }

    /**
     * @brief Set the camera area of the goal item cutscene and the return step of the flag camera.
     * @param pArea The camera area.
     * @param returnStep The return step of the flag camera.
     */
    void setCameraArea(al::AreaObj* pArea, f32 returnStep) {
        mCameraArea = pArea;
        mFlagReturnStep = returnStep;
    }

    /**
     * @brief Get the number of finished main scenarios.
     * @return The finished main scenario count.
     */
    s32 getFinishedMainNum() const { return mFinishedMainNum; }

    /**
     * @brief Set the actor that landed with the goal item.
     * @param pActor The landed actor.
     */
    void setLandedActor(al::LiveActor* pActor) { mLandedActor = pActor; }

    /**
     * @brief Check whether the lighthouse is inked.
     * @return True while inked.
     */
    bool isInked() const { return mIsInked; }
    void logIslandVisit(s32 islandId, bool, bool isPlayTime);

private:
    void clearInk();
    void startReturnSetDir(al::CameraPoserFixActor* pPoser, s32 step);
    void finishGoalItemCutscene();

    al::LiveActor* mInk;                                 // 0x150
    al::PlacementId* mPlacementId;                       // 0x158
    s32 _160 = 0;
    sead::PtrArray<GoalItem> mGoalItems;                 // 0x168
    InkPatch* mInkPatch = nullptr;                       // 0x178
    s32 mInkPatchDisappearStep;                          // 0x180
    s32 mInkPatchWaitStep;                               // 0x184
    s32 mInkLinkId = -1;                                 // 0x188
    al::CameraTicket* mCamera = nullptr;                 // 0x190
    s32 mCameraReturnStep = 0;                           // 0x198
    s32 mCameraInterpoleStep = 20;                       // 0x19c
    s32 mFlagCameraInStep = 60;                          // 0x1a0
    s32 mFlagCameraHoldStep = 180;                       // 0x1a4
    s32 mFlagCameraOutStep = 60;                         // 0x1a8
    s32 mFlagReturnStep = 0;                             // 0x1ac
    s32 mCurrentScenarioId = 0;                          // 0x1b0
    s32 mActiveScenario = 0;                             // 0x1b4
    s32 mFinishedMainNum = 0;                            // 0x1b8
    s32 mFinishedNum = 0;                                // 0x1bc
    s32 mInkFinishedNum;                                 // 0x1c0
    bool mIsCameraSequenceStarted = false;               // 0x1c4
    bool mIsInked = false;                               // 0x1c5
    bool mIsScenarioAnimPending = false;                 // 0x1c6
    bool mIsLighthouseShone = false;                     // 0x1c7
    bool mIsInkPatchKeepModel = false;                   // 0x1c8
    bool mIsReturnSetDir = false;                        // 0x1c9
    bool mIsFinishBlocked = false;                       // 0x1ca
    bool mIsGoalItemArrived = false;                     // 0x1cb
    bool mIsFlagAppeared = false;                        // 0x1cc
    bool mIsInkMeNot = false;                            // 0x1cd
    bool mIsInkPillarKilled = false;                     // 0x1ce
    bool mIsSkipped;                                     // 0x1cf
    bool mIsReturnByStep;                                // 0x1d0
    bool mIsNoReturn;                                    // 0x1d1
    bool _1d2 = false;
    bool mIsJumpedToFinish;                              // 0x1d3
    bool _1d4;
    bool mIsLookAtPointSet = false;                      // 0x1d5
    bool mIsSinkedItemKillRequested = false;             // 0x1d6
    bool _1d7 = false;
    bool mIsDisasterPhase = false;                       // 0x1d8
    sead::Vector3f mLandedPos;                           // 0x1dc
    f32 mReturnDistance;                                 // 0x1e8
    f32 mReturnAngleH = 0.0f;                            // 0x1ec
    f32 mReturnAngleV = 20.0f;                           // 0x1f0
    f32 mReturnDirRate = 0.14f;                          // 0x1f4
    f32 mReturnDistanceRate = 0.12f;                     // 0x1f8
    f32 mReturnOffsetY;                                  // 0x1fc
    s32 mReturnEndStep;                                  // 0x200
    s32 mReturnStep;                                     // 0x204
    f32 mDistanceOffset = 0.0f;                          // 0x208
    f32 mHeightOffset = 0.0f;                            // 0x20c
    al::AreaObj* mCameraArea = nullptr;                  // 0x210
    const char* mGuideMessage = nullptr;                 // 0x218
    al::LiveActor* mLandedActor = nullptr;               // 0x220
    FlingPole* mFlingPole = nullptr;                     // 0x228
    sead::Vector3f mFlingPoleOffset = sead::Vector3f::zero;  // 0x230
    FlagState mPrevFlagState = FlagState_None;           // 0x23c
    FlagState mFlagState = FlagState_None;               // 0x240
    bool mIsLodEffect[3] = {};                           // 0x244
    bool mIsPhaseOffsetSet;                              // 0x247
    LookAtPointInterpole* mLookAtInterpole = nullptr;    // 0x248
    SinkedItem* mSinkedItem = nullptr;                   // 0x250
    al::FunctorBase* mFadeInDoneFunctor = nullptr;       // 0x258
    bool mIsInkFadedOut;                                 // 0x260
    al::LayoutInitInfo mLayoutInitInfo;                  // 0x268
    s32 mUnlockedPhase = 0;                              // 0x2d8
    GoalItem* mGoalItem = nullptr;                       // 0x2e0
    sead::Vector3f mSinkedItemOffset;                    // 0x2e8
};

static_assert(sizeof(Lighthouse) == 0x2f8);
