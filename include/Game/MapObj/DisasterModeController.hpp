#pragma once

#include <attributes.h>
#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Boss/SuperBowserShell.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class AreaObj;
class FunctorBase;
class IUseSceneObjHolder;
class LayoutActor;
class LiveActorGroup;
}  // namespace al

class DemoAnimatic;
class DemoCutscene;
class DisasterBlockDirector;
class DisasterLightning;
class DisasterSpikeDirector;
class GameDataHolderAccessor;
class GameSkyProjection;
class GigaBellManager;
class GraphicsAreaController;
class SplatterPlotter;
class SuperBowser;

/**
 * @brief Controls Bowser's Fury disaster mode: the cycle of peace (prosperity), the Black Sun
 * rising (anticipation) and Fury Bowser's rampages (disaster), together with the sky, rain,
 * lightning, music and cutscenes that go with it.
 */
class DisasterModeController : public al::LiveActor, public al::ISceneObj {
public:
    /// Step of the disaster cycle, sent to the state listeners.
    enum class State : s32 {
        ProsperityTransitionStart = 0,
        Normal = 1,
        ProsperityTransitionEnd = 2,
        Prosperity = 3,
        AnticipationTransition = 4,
        Anticipation = 5,
        DisasterTransitionStart = 6,
        Disaster = 7,
        DisasterTransitionEnd = 8,
        DisasterStart = 9,
        DisasterLong = 10,
        RainStart = 11,
    };

    /// Difficulty of Fury Bowser's rampage.
    enum Mode : s32 {
        Mode_Normal = 0,
        Mode_Hard = 1,
        Mode_SuperHard = 2,
    };

    /// Music change queued for the next updates.
    enum BGM_REQUEST : s32 {
        BGM_REQUEST_NONE = 0,
        BGM_REQUEST_DISASTER = 1,
        BGM_REQUEST_STOP_DISASTER = 2,
        BGM_REQUEST_PROSPERITY = 3,
        BGM_REQUEST_LEAVE_CUTSCENE = 4,
        BGM_REQUEST_END_INSTANTLY = 5,
        BGM_REQUEST_END = 6,
    };

    /// Durations and difficulty of one prosperity/disaster cycle.
    struct DisasterModeFlowNode {
        void set(s32 prosperityFrames, s32 disasterFrames, bool isMini, bool isHard);

        s32 prosperityFrames = 0;
        s32 disasterFrames = 0;
        bool isMini = false;
        bool isHard = false;
    };

    /// Receives the steps of the disaster cycle.
    class IUseEventReceiver {
    public:
        virtual void onDisasterModeStateChange(State) = 0;
    };

    /**
     * @brief Fixed-capacity list of the state listeners, emptied on destruction.
     */
    template <typename T, s32 N>
    class FixedListenerArray : public sead::PtrArray<T> {
    public:
        FixedListenerArray() : sead::PtrArray<T>(N, mWork) {}

        /** @brief Empties the list. */
        ~FixedListenerArray() { this->clear(); }

    private:
        T* mWork[N];
    };

    using StateListenerArray = FixedListenerArray<IUseEventReceiver, 1024>;

    explicit DisasterModeController(const char* pName);

    /**
     * @brief Register an object to be notified of the steps of the disaster cycle.
     * @param listener The object to notify.
     */
    void registerStateListener(IUseEventReceiver* listener) {
        if (!mStateListeners.isFull()) {
            mStateListeners.pushBack(listener);
        }
    }

    static DisasterModeController* tryGetController(const al::IUseSceneObjHolder* pUser);
    void forceKillSuperBowserAttacks();
    void forceKillSuperBowserLaser();
    void setSuperBowserV2(bool isV2);
    void setSkyEnable(bool isEnable);
    NOINLINE void setSkyDisaster();
    void setSkyProsperity();
    void setSkyFinalCutscene();
    ~DisasterModeController() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void jumpToRain();
    void initFlow();
    static bool isLastBowserBattle(GameDataHolderAccessor accessor);
    bool isReadyForDemo() const;
    void pauseWithSuperBowser();
    void resumeWithSuperBowser();
    State getState();
    s32 getStateFrame();
    bool isPhaseZero(GameDataHolderAccessor accessor);
    bool isRepelling();
    bool isBowserHidden();
    SuperBowser* tryCreateSuperBowser(const al::ActorInitInfo& rInfo);
    void initAfterPlacement() override;
    bool isPhasePart(s32 part, bool isLockCountIncremented) const;
    void clearPostBossPeaceFrames();
    void tryStartAnticipationMusic(bool isDelayed, bool isLong);
    void appear() override;
    void startBgmRequest(BGM_REQUEST request, bool isForce);
    void updateBgmRequest();
    bool calcHardMode();
    bool calcSuperHardMode();
    void updateAmbientSE();
    bool isRaining();
    bool isDisasterNerve();
    bool isDisasterStarting();
    bool needToPlayFirstDisasterModeCutscene();
    bool tryPlayFirstDisasterModeCutscene();
    void endFirstAppearCutsceneFunc();
    void setState(s32 startFrames, bool isDisaster, s32 pauseFrames, bool isFromCutscene);
    void cancelFirstAppearCutsceneFunc();
    void disasterModeCutsceneTransitionFunc();
    void setTransitionFrameRate(f32 wipeInRate, f32 wipeOutRate);
    void control() override;
    void checkNoDisasterIslandAreas();
    void updateRainEffects();
    bool isRainEffectsOn();
    void forceDisasterForeshadowOff(bool isForce, s32 delay);
    void movementPaused(bool isPaused) override;
    static s32 findAreaId(const al::LiveActor* pActor, sead::Vector3f offset);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void begin(bool isWithDemo);
    void beginWithDemo();
    void beginImmediate(bool isWithDemo);
    void beginImmediateAndNeverEndDebug();
    void beginAndNeverEndDebug();
    void resetDisasterTimer();
    void end();
    void endForeshadow();
    void endByTime();
    void endWithDemo();
    void endAndNeverBeginDebug();
    void endInstantly(bool isStopBgm, bool isDisappear);
    void endImmediate();
    HIDDEN static void onBowserDisappeared(void* pController);
    void pause(bool isByUser);
    void resume(bool isForce);
    void togglePause();
    void laserAttack();
    f32 disasterPercentage();
    bool isShellMax();
    bool isAnticipation();
    bool isWhiteOut();
    bool isWipeActive();
    al::LiveActor* getShell();
    void stopFireballs();
    void startFireballs();
    void useWipePlain();
    void useWipeBowser();
    void tryDrawDisasterModeTimer();
    s32 calcFramesOfProsperity() const;
    s32 calcProsperityStartAdditionalFramesMax();
    void startBGM();
    void tryStartPhase0Music();
    void oneTimeAutoTrigger();
    void exeWait();
    void exeProsperityTransitionWipeIn();
    void tryChangeActorWetMaterial(bool isWet);
    void startWhiteTransitionToProsperity();
    void exeProsperityTransitionWipeOut();
    bool isHardMode() const;
    bool isSuperHardMode() const;
    bool isSuperBowserLeaving();
    bool setIsFirstDisappearCutscene(bool isFirst);
    bool setPostCutsceneDisasterFreezeTime();
    void clearTimeJumpFlags();
    void skip(s32 seconds);
    void setForceMode(bool isForce, Mode mode);
    void preDisasterMode();
    void calcMode();
    void tryIncrementFlow();
    void disableRain();
    void stopRain();
    bool tryApplyDoubleTime();
    bool isDoubleTime() const;
    void exeProsperity();
    void startRain();
    s32 getForeshadowStartFrame();
    void exeAnticipationTransition();
    HIDDEN static void onShellPreLaunched(void* pController);
    void exeFirstAppearCutscene();
    void exeLeaveCutscene();
    void exeAnticipation();
    void exeAnticipationFast();
    HIDDEN static void onShellLaunched(void* pController);
    void exeDisasterTransitionInstant();
    void tryStartChaseMusic();
    void exeDisasterTransitionWipeIn();
    void exeDisasterTransitionWipeOut();
    void exeDisasterTransitionWipeInDemo();
    void exeDisasterTransitionWipeOutDemo();
    void exeDisaster();
    void startLightning();
    HIDDEN static void onBowserDisappearStart(void* pController);
    void updateLightning();
    void exeNoOp();
    void exePostBattlePeaceDelay();
    bool isInDisasterOrInstant() const;
    void triggerAnticipationSwitch();
    void jumpToForeshadow();
    void tryJumpToRainWithFlash();
    void randomizeLightningInterval();
    void stopLightning();

    /**
     * @brief Get the name of the scene object.
     * @return The name of the scene object.
     */
    const char* getSceneObjName() const override { return "DisasterModeController"; }

    /**
     * @brief Get the step of the disaster cycle.
     * @return The step of the disaster cycle.
     */
    s32 getFlowStep() const { return static_cast<s32>(mState); }

    /**
     * @brief Access the director of the disaster blocks.
     * @return The block director.
     */
    DisasterBlockDirector* getBlockDirector() const { return mBlockDirector; }

    /** @brief Make the next disaster start when a goal item is collected. */
    void setGoalItemDisasterTrigger() { mGoalItemDisasterTrigger = true; }

    /**
     * @brief Set whether the disaster cycle may progress (cleared while a player is in a cloud
     * bonus area).
     * @param isEnable Whether the disaster cycle may progress.
     */
    void setDisasterProgressEnable(bool isEnable) { mIsDisasterProgressEnable = isEnable; }

    /**
     * @brief Set the callback run once the screen faded back in after ending disaster mode.
     * @param pFunctor The callback.
     */
    void setFadeInDoneFunctor(al::FunctorBase* pFunctor) { mFadeInDoneFunctor = pFunctor; }

    /**
     * @brief Access the Fury Bowser actor.
     * @return The Fury Bowser actor, or nullptr when absent.
     */
    SuperBowser* getSuperBowser() const { return mpSuperBowser; }

    SuperBowserShell* getSuperBowserShell() const { return mShell; }

    /**
     * @brief Count the frames elapsed in disaster mode, including the saved offset.
     * @return The elapsed disaster-mode frames.
     */
    s32 calcDisasterFrames() const { return mDisasterFrames + mDisasterFramesOffset; }

    /**
     * @brief Access the director of the disaster spikes.
     * @return The spike director.
     */
    DisasterSpikeDirector* getSpikeDirector() const { return mSpikeDirector; }

    /**
     * @brief Check whether disaster mode is active.
     * @return True while disaster mode is active.
     */
    bool isDisasterMode() const { return mIsDisasterMode; }

    /**
     * @brief Check whether objects should show their disaster mode animations.
     * @return Whether the disaster mode animations are active.
     */
    bool isDisasterModeAnim() const { return mIsDisasterModeAnim; }

    /**
     * @brief Count the frames of peace elapsed before the next disaster.
     * @return The elapsed peace frames.
     */
    s32 getPeaceFrames() const { return mPeaceFrames; }

    /**
     * @brief Count the frames of rain that precede the disaster.
     * @return The pre-disaster rain frames.
     */
    s32 getPreRainFrames() const { return mPreRainFrames; }

    /**
     * @brief Get the disaster-mode frames elapsed (without the saved offset).
     * @return The elapsed disaster-mode frames.
     */
    s32 getDisasterFrames() const { return mDisasterFrames; }

    /**
     * @brief Jump the disaster timer to a frame.
     * @param frames The new elapsed disaster-mode frames.
     */
    void setDisasterFrames(s32 frames) {
        mDisasterFrames = frames;
        mDisasterFramesSync = frames;
    }

    /**
     * @brief Set the saved disaster-mode frame offset.
     * @param frames The new offset.
     */
    void setDisasterFramesOffset(s32 frames) { mDisasterFramesOffset = frames; }

    /**
     * @brief Get the saved disaster-mode frame offset.
     * @return The offset.
     */
    s32 getDisasterFramesOffset() const { return mDisasterFramesOffset; }

    /**
     * @brief Check whether the Black Sun only floats in place instead of rising.
     * @return True while the Black Sun floats.
     */
    bool isBlackSunFloating() const { return mIsBlackSunFloating; }

    /**
     * @brief Set whether the black sun floats.
     * @param isFloating Whether the black sun floats.
     */
    void setBlackSunFloating(bool isFloating) { mIsBlackSunFloating = isFloating; }

    /**
     * @brief Mark disaster mode as ended immediately by a goal item.
     */
    void setGoalItemEndImmediate() { _249 = true; }

    /**
     * @brief Mark the running demo as skipped.
     */
    void setDemoSkipped() { _385 = true; }

    /**
     * @brief Check whether the pre-disaster mode setup is done.
     * @return True once done.
     */
    bool isPreDisasterModeDone() const { return mIsPreDisasterModeDone; }

    /**
     * @brief Check whether the disaster timer is stopped.
     * @return True while the timer is stopped.
     */
    bool isTimeStopped() const { return mIsTimeStopped; }

    /**
     * @brief Check whether the disaster foreshadowing (the calm before Fury Bowser) is active.
     * @return True while the disaster is foreshadowed.
     */
    bool isDisasterForeshadow() const { return mIsDisasterForeshadow; }

    /**
     * @brief Access the second actor taking part in the disaster demos (the Black Sun).
     * @return The actor, or nullptr when absent.
     */
    al::LiveActor* getDemoSubActor() const { return mShell; }

private:
    /**
     * @brief Count the frames of a whole prosperity/disaster cycle.
     * @return The cycle frames.
     */
    s32 calcCycleFrames() const {
        return mProsperityTransitionFrames + mPeaceFrames + mAnticipationTransitionFrames +
               mAnticipationFrames + mDisasterTransitionFrames;
    }

    /**
     * @brief Tell the state listeners about a step of the disaster cycle.
     * @param state The step.
     */
    void notifyStateChange(State state) {
        if (mStateListeners.isEmpty()) {
            return;
        }

        IUseEventReceiver** it = mStateListeners.dataEnd() - 1;
        while (true) {
            IUseEventReceiver** current = it--;
            (*current)->onDisasterModeStateChange(state);
            if (current == mStateListeners.dataBegin()) {
                break;
            }
        }
    }

    /**
     * @brief Move to a step of the disaster cycle and tell the state listeners.
     * @param state The step.
     */
    void changeState(State state) {
        mState = state;
        notifyStateChange(state);
    }

    inline void showProsperitySky();
    inline void endDisasterPhase();
    inline void stopForeshadowBgm();
    inline void startChaseBgm();
    inline void appearSuperBowser();
    inline bool isGigaBellLockCountIncremented() const;

    f32 mWipeInFrameRate = 1.0f;                                  // 0x150
    f32 mWipeOutFrameRate = 1.0f;                                 // 0x154
    DisasterSpikeDirector* mSpikeDirector = nullptr;              // 0x158
    DisasterBlockDirector* mBlockDirector;                        // 0x160
    GraphicsAreaController* mGraphicsAreaController = nullptr;    // 0x168
    al::LiveActorGroup* mAllActorGroup = nullptr;                 // 0x170
    GameSkyProjection* mSkyLake = nullptr;                        // 0x178
    GameSkyProjection* mSkyDisaster = nullptr;                    // 0x180
    GameSkyProjection* mSkyDisasterHard = nullptr;                // 0x188
    GameSkyProjection* mSkyDisasterSuperHard = nullptr;           // 0x190
    GameSkyProjection* mSkyFinalCutscene = nullptr;               // 0x198
    s32 _1a0;
    bool mIsProsperityTransitionNeeded = false;                   // 0x1a4
    bool mIsSkipAnticipationFade = true;                          // 0x1a5
    s32 mProsperityTransitionFrames = 0;                          // 0x1a8
    s32 mPeaceFrames = 0;                                         // 0x1ac
    s32 mAnticipationTransitionFrames = 0;                        // 0x1b0
    s32 mAnticipationFrames = 0;                                  // 0x1b4
    s32 mDisasterTransitionFrames = 0;                            // 0x1b8
    s32 mDisasterDurationFrames = 0;                              // 0x1bc
    s32 mDisasterFramesSync;                                      // 0x1c0
    s32 mDisasterFrames = 0;                                      // 0x1c4
    s32 mStartFrames = -1;                                        // 0x1c8
    s32 mDisasterElapsedFrames = 0;                               // 0x1cc
    s32 _1d0 = 0;
    s32 _1d4 = 1800;
    s32 mFreezeFrames = 0;                                        // 0x1d8
    f32 mAnticipationPercentage;                                  // 0x1dc
    s32 mShellPhaseCount;                                         // 0x1e0
    SuperBowser* mpSuperBowser = nullptr;                         // 0x1e8
    SuperBowserShell* mShell = nullptr;                           // 0x1f0
    bool _1f8 = true;
    bool mIsDisasterMode = false;                                 // 0x1f9
    bool mIsDisasterModeAnim = false;                             // 0x1fa
    bool mIsBlackSunFloating = false;                             // 0x1fb
    bool mIsForeshadowOff = false;                                // 0x1fc
    s32 mForeshadowOffDelay = 0;                                  // 0x200
    bool mIsDisasterProgressEnable = true;                        // 0x204
    void* _208 = nullptr;
    void* _210 = nullptr;
    void* _218 = nullptr;
    bool _220 = false;
    bool mIsStartInDisasterMode = false;                          // 0x221
    DemoCutscene* mFirstAppearDemo = nullptr;                     // 0x228
    DemoAnimatic* mTimeExpireDemo = nullptr;                      // 0x230
    bool mIsInFirstAppearDemo = false;                            // 0x238
    bool mIsInDisappearDemo = false;                              // 0x239
    bool mIsNeverEnd = false;                                     // 0x23a
    bool mIsTimeStopped = false;                                  // 0x23b
    bool mGoalItemDisasterTrigger = false;                        // 0x23c
    bool mIsHideShell = false;                                    // 0x23d
    s32 mFireballStopCount = 0;                                   // 0x240
    s32 mDoubleTimeDelay = -1;                                    // 0x244
    bool _248 = true;
    bool _249 = false;
    al::LayoutActor* mWipeLayout = nullptr;                       // 0x250
    bool mIsWipePlain = true;                                     // 0x258
    bool mIsDrawDebugTimer = false;                               // 0x259
    bool mIsOneTimeAutoTrigger = false;                           // 0x25a
    bool mIsDisasterForeshadow = false;                           // 0x25b
    bool mIsJumpToForeshadow = false;                             // 0x25c
    bool mIsJumpToRain = false;                                   // 0x25d
    bool mIsTimeJumped = false;                                   // 0x25e
    al::FunctorBase* mJumpToRainFunctor = nullptr;                // 0x260
    bool mIsRainDisabled = false;                                 // 0x268
    s32 mPostBossPeaceFrames = 0;                                 // 0x26c
    bool mIsEnding = false;                                       // 0x270
    bool mIsNeedFirstAppearDemo = false;                          // 0x271
    sead::PtrArray<DisasterLightning> mLightnings;                // 0x278
    s32 mLightningNum = 8;                                        // 0x288
    bool mIsLightning = false;                                    // 0x28c
    s32 mLightningInterval = 0;                                   // 0x290
    s32 mLightningTimer = 0;                                      // 0x294
    bool mIsWetMaterial = false;                                  // 0x298
    bool mIsSkyEnable = true;                                     // 0x299
    bool mIsSkyDisaster = false;                                  // 0x29a
    State mState = State::Prosperity;                             // 0x29c
    sead::Matrix34f mRainMtx = sead::Matrix34f::ident;            // 0x2a0
    s32 mDisasterFramesOffset = 0;                                // 0x2d0
    s32 mPreRainFrames = 3600;                                    // 0x2d4
    s32 mDisasterTransitionDemoFrame = 690;                       // 0x2d8
    s32 mRepelDayTransitionFrame = 710;                           // 0x2dc
    s32 mDisappearDayTransitionFrame = 820;                       // 0x2e0
    bool mIsSkipProsperityWipe = false;                           // 0x2e4
    bool mIsForeshadowOffForced = false;                          // 0x2e5
    bool mIsForceMode = false;                                    // 0x2e6
    Mode mForceMode = Mode_Normal;                                // 0x2e8
    SplatterPlotter* mSplatterPlotter;                            // 0x2f0
    bool mIsDoubleTime = false;                                   // 0x2f8
    al::AreaObj* mPlessieTunnelArea = nullptr;                    // 0x300
    bool mIsAllDisasterShinesRemaining = false;                   // 0x308
    bool mIsAllNekoShinesRemaining = false;                       // 0x309
    bool mIsBeginWithDemo = false;                                // 0x30a
    Mode mMode = Mode_Normal;                                     // 0x30c
    bool mIsRaining = false;                                      // 0x310
    State mAmbientSeState = State::Prosperity;                    // 0x314
    bool mIsPreDisasterModeDone = false;                          // 0x318
    s32 mFlowNodeNum = 0;                                         // 0x31c
    DisasterModeFlowNode mFlowNodes[6];                           // 0x320
    s32 mFlowIndex = 0;                                           // 0x368
    al::FunctorBase* mFadeInDoneFunctor = nullptr;                // 0x370
    GigaBellManager* mGigaBellManager = nullptr;                  // 0x378
    BGM_REQUEST mBgmRequest = BGM_REQUEST_NONE;                   // 0x380
    bool mIsBgmPlaying = false;                                   // 0x384
    bool _385 = false;
    bool mIsPausedByUser = false;                                 // 0x386
    s32 mBgmRequestDelay = 0;                                     // 0x388
    StateListenerArray mStateListeners;                           // 0x390
};

static_assert(sizeof(DisasterModeController) == 0x23a0);

using DisasterModeStateListener = DisasterModeController::IUseEventReceiver;
