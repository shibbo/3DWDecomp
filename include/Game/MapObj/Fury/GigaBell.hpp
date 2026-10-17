#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class AreaObj;
class CameraTicket;
class CollisionObj;
class HitSensor;
class Nerve;
class SensorMsg;
class WipeSimple;
}  // namespace al

class CounterLockGigaBell;
class DemoAnimatic;
class DemoCutscene;
class DemoSkipLayout;
class DummyCameraTarget;
class GigaBellBindPuppeteer;
class GigaBellManager;
class GigaBellPedestal;
class IUsePlayerPuppet;
class ItemStatePopUpAbove;
class PlayerActor;
class SuperBowser;

/**
 * @brief Giga Bell of Bowser's Fury: locked by Fury Bowser until enough Cat Shines are
 * collected, it turns the player into Giga Cat Mario when rung.
 */
class GigaBell : public al::LiveActor {
public:
    /// Lock state requested by the GigaBellManager.
    enum class LockState : s32 {
        Locked = 0,
        LockedDisasterMode = 1,
        LockedSufficientGoalItems = 2,
        Unlocked = 3,
    };

    explicit GigaBell(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void setBasePosition(sead::Vector3f basePos);
    void setInk(bool isInk);
    void updateCollisionMtx();
    void appear() override;
    void startReset();
    void startDemoActor(s32 demoId) override;
    void endDemoActor(s32 demoId) override;
    void appearPopUpFront();
    void appearPopUpAbove();
    void appearPopUpAboveSilent();
    void reappearInBattle();
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isUnlocked();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool canRing();
    bool isMsgPlayerAttack(const al::SensorMsg* pMsg);
    void collectGigaBellBossFight(al::HitSensor* pOther, al::HitSensor* pSelf);
    void plessieChaseHit(al::HitSensor* pOther, al::HitSensor* pSelf);
    bool collectGigaBell(al::HitSensor* pOther, al::HitSensor* pSelf);
    void makeActorDead() override;
    void control() override;
    void startClipped() override;
    void endClipped() override;
    void clearUnlockSe();
    void exeWait();
    void exePreExplainCutscene();
    void exeExplainCutscene();
    void trySetPreDemoKoura();
    void exeUnlocked();
    void exeLockedSufficientGoalItems();
    void setDamaging(bool isDamaging);
    void startWaitUnravel();
    void exeLockedDisasterMode();
    void exeLocked();
    void exeLockedPlayerGiga();
    void trySetNerve(const char* pName, const al::Nerve* pNerve);
    void exeRinging();
    bool activateCamera(al::LiveActor* pTarget, f32 distance, f32 angleV, f32 angleH,
                        f32 offsetY, f32 interpoleFrame, bool isDemo);
    f32 calculateAngleHToShell();
    void deactivateCamera();
    void exeCollectingCameraPending();
    void exeCollecting();
    void exeCollectingBossBattle();
    void exeReappearInBattle();
    void exeWaitInBattle();
    void exeWaitReviveInBattle();
    void exeBattleDefeatDemo();
    void exePopUpAbove();
    void exePreUnlockCutscene();
    void addPlayerItemsToDemo();
    void exeUnlockCutscene();
    void startReturnCutscene(bool isReset);
    void exePreResetCutscene();
    void exeResetCutscene();
    void exeResetCutsceneEnd();
    void exePlessieChase();
    void exePlessieChaseHit();
    void exeTransitionSleep();
    void exeTransitionWakeUp();
    void exeDebugCollectWait();
    void exeCollectDemo();
    void startUnlockCutscene();
    void trySetLockState(LockState state);
    void startLockedPlayerGiga();
    void forceUnlock();
    void setCameraAngleH(f32 angleH);
    void setCameraDistance(f32 distance);
    s32 getPhase();
    void setIsBellInCutscene(bool isInCutscene);
    sead::Vector3f getRespawnPlayerPos();
    sead::Vector3f getPlessieChasePos();
    sead::Vector3f getPlessieChaseFront();
    bool hasBeenHitPlessieChase();
    void startPlessieChase(SuperBowser* pBowser);
    void updatePlessieChase(s32 index);
    void doPlessieChaseHitEffect();
    void plessieChaseReset();
    bool isInUnlockCutscene();
    void setDamagingHalf();

    /**
     * @brief Set the pedestal the bell stands on.
     * @param pPedestal The pedestal.
     */
    void setPedestal(GigaBellPedestal* pPedestal) { mPedestal = pPedestal; }

    /**
     * @brief Set the manager owning the bell.
     * @param pManager The manager.
     */
    void setManager(GigaBellManager* pManager) { mManager = pManager; }

private:
    bool mIsDebugCollect = false;
    void* _148 = nullptr;
    al::AreaObj* mExplainArea = nullptr;
    bool mIsCameraActive = false;
    bool mHasRestartPos = false;
    bool mIsRevivableInBattle = true;
    sead::Vector3f mRestartPos = sead::Vector3f::zero;
    sead::Vector3f mRestartFront = sead::Vector3f::ez;
    s32 _174;
    DemoAnimatic* mGetDemo = nullptr;
    DemoCutscene* mExplainCutscene = nullptr;
    GigaBellManager* mManager = nullptr;
    CounterLockGigaBell* mCounterLock = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mBasePos = sead::Vector3f::zero;
    f32 mBaseScale = 1.0f;
    al::CameraTicket* mCameraTicket = nullptr;
    DummyCameraTarget* mCameraTarget = nullptr;
    IUsePlayerPuppet* mPuppet = nullptr;
    bool mIsUnlockedOnce = false;
    ItemStatePopUpAbove* mStatePopUpAbove = nullptr;
    bool mIsBellInCutscene = false;
    bool mIsStartUnlockSe = false;
    s32 mLayerId = -1;
    s32 mCutsceneId = -1;
    s32 mIslandId = -1;
    sead::Matrix34f* mConversationMtx = nullptr;
    PlayerActor* mPlayer = nullptr;
    GigaBellBindPuppeteer* mBindPuppeteer = nullptr;
    f32 mPlayerScale = 1.0f;
    al::CollisionObj* mCollisionNormal = nullptr;
    al::CollisionObj* mCollisionDamage = nullptr;
    al::CollisionObj* mCollisionHalf = nullptr;
    sead::Matrix34f mCollisionMtx = sead::Matrix34f::ident;
    DemoCutscene* mGetCutscene = nullptr;
    al::WipeSimple* mWipe = nullptr;
    DemoSkipLayout* mDemoSkipLayout = nullptr;
    bool mIsDemoSkipped = false;
    bool mIsCameraReturn = false;
    bool mIsSufficientGoalItems = false;
    bool mIsInDemo = false;
    GigaBellPedestal* mPedestal = nullptr;
    al::HitSensor* mCollectSensor = nullptr;
    al::AreaObj* mCameraArea = nullptr;
    s32 mBallHitCooldown = 0;
    s32 mUnlockDemoSwitchTimer = 0;
    al::LiveActor* mDemoKoura = nullptr;
    bool mHasPlessieChasePos = false;
    sead::Vector3f mPlessieChasePos = sead::Vector3f::zero;
    sead::Vector3f mPlessieChaseFront = sead::Vector3f::ez;
    const sead::Matrix34f* mBowserJointMtx = nullptr;
    sead::Vector3f mPlessieChaseOffset = sead::Vector3f::zero;
    sead::Vector3f mPlessieChasePrevOffset = sead::Vector3f::zero;
    bool mIsHitPlessieChase = false;
};

static_assert(sizeof(GigaBell) == 0x2e8);
