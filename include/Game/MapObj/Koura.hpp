#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActorCollisionController;
class ActorSensorController;
class ComboCounter;
}  // namespace al

class ActorStateRouteDokanMove;
class ItemStatePlayerHold;
class KouraBindPuppeteer;

/// Countdown timers (in frames) that temporarily disable some of the shell's interactions.
struct KouraTimer {
    /** @brief Clears every timer. */
    void reset() {
        mInvalidTrampleTime = 0;
        mInvalidRouteDokanTime = 0;
        mInvalidHoldTime = 0;
        mInvalidAttackTime = 0;
        mInvalidBlowTime = 0;
    }

    /** @brief Counts every running timer down by one frame. */
    void update() {
        if (mInvalidTrampleTime > 0) {
            mInvalidTrampleTime--;
        }

        if (mInvalidRouteDokanTime > 0) {
            mInvalidRouteDokanTime--;
        }

        if (mInvalidHoldTime > 0) {
            mInvalidHoldTime--;
        }

        if (mInvalidAttackTime > 0) {
            mInvalidAttackTime--;
        }

        if (mInvalidBlowTime > 0) {
            mInvalidBlowTime--;
        }
    }

    s32 mInvalidTrampleTime = 0;     // 0x0
    s32 mInvalidRouteDokanTime = 0;  // 0x4
    s32 mInvalidHoldTime = 0;        // 0x8
    s32 mInvalidAttackTime = 0;      // 0xc
    s32 mInvalidBlowTime = 0;        // 0x10
};

static_assert(sizeof(KouraTimer) == 0x14);

/// Movement state of the player holding the shell, sampled while it is carried.
enum class KouraPlayerMoveState : s32 {
    None = 0,
    Wait = 1,
    Dash = 2,
    DashFast = 3,
};

/// Green shell that can be kicked, carried and thrown, or ridden from the inside.
class Koura : public al::LiveActor {
public:
    explicit Koura(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableRouteDokan() const;
    bool isPlayerInside() const;
    bool isAttack() const;
    void startBlow(const al::HitSensor* pSelf, const al::HitSensor* pOther);
    void setMoveDir(const sead::Vector3f& rDir);
    void startBreak();
    void resetTimer();
    void startKouraSlide(const sead::Vector3f& rDir);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableKick() const;
    bool isSlide() const;
    void startKill();
    void startMove(const sead::Vector3f& rDir, f32 speed, f32 rotateSpeed);
    bool isEnableTrampleStop() const;
    bool isEnableHold() const;
    bool isHold() const;
    void endHold();
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void appear() override;
    void control() override;
    bool tryBreak();
    void updateCollider() override;
    void startKillBySwitch();
    void appearFromNokonoko(al::LiveActor* pNokonoko, bool isUpper);
    void disappearByNokonoko(al::LiveActor* pNokonoko);
    bool isAttachableWithNokonoko();
    void receivedBindCancel(KouraBindPuppeteer* pPuppeteer, al::HitSensor* pSensor, bool isKill);
    void updateRotatePose();
    bool hideActor() override;
    void startEffectHitCollision();
    bool isFixWaterSurface(f32 depth) const;
    bool doFall(f32 bounceRate, f32 scaleH);
    void doMove();
    bool isSlideWithoutPlayer() const;
    bool isInRouteDokan() const override;
    bool isJumpStart() const;
    void exeWait();
    void exeWaitForBind();
    void exeUpper();
    void exeHold();
    void startHold();
    void exeWaitRelease();
    void exeRelease();
    void exeBindStart();
    void exeKouraSlide();
    void doKouraInputProc();
    void exeKouraStop();
    void exeKouraWait();
    void exeKouraJump();
    void exeKouraJumpBreak();
    void exeBindEnd();
    void exeBlow();
    void exeWaitBindEndForKill();
    void exeRouteDokan();
    void exeRouteDokanEnd();
    void exeDemoKill();
    virtual const char* getArchiveName() const;
    virtual void onHitWall() {}

private:
    void tryDisappearGuide();
    void tryAppearGuide();
    void tryEnableIslandWarp();

    al::ActorSensorController* mSensorController = nullptr;        // 0x148
    al::ActorCollisionController* mCollisionController = nullptr;  // 0x150
    u8 _158[0x188 - 0x158];
    KouraBindPuppeteer* mBindPuppeteer = nullptr;          // 0x188
    KouraBindPuppeteer** mBindPuppeteers = nullptr;        // 0x190
    ItemStatePlayerHold* mHoldState;                       // 0x198
    al::HitSensor* mPlayerSensor = nullptr;                // 0x1a0
    al::ComboCounter* mComboCounter;                       // 0x1a8
    KouraTimer* mTimer = nullptr;                          // 0x1b0
    s32 mLiftingCount = 0;                                 // 0x1b8
    s32 mClippedFrame = 0;                                 // 0x1bc
    f32 mRotateSpeed = 0.0f;                               // 0x1c0
    s32 mSlideTime = 0;                                    // 0x1c4
    s32 mSlideTimeMax = 0;                                 // 0x1c8
    f32 mMoveSpeed = -1.0f;                                // 0x1cc
    f32 mRotateDegree = 0.0f;                              // 0x1d0
    sead::Vector3f mUp = {0.0f, 1.0f, 0.0f};               // 0x1d4
    sead::Vector3f mGroundNormal = {0.0f, 1.0f, 0.0f};     // 0x1e0
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;   // 0x1ec
    bool mIsOnGround = false;                              // 0x21c
    bool mIsInWater = false;                               // 0x21d
    bool mIsJumpBoost = false;                             // 0x21e
    bool mIsBindGoal = false;                              // 0x21f
    bool mIsHiddenByBind;                                  // 0x220
    KouraPlayerMoveState mPlayerMoveState = KouraPlayerMoveState::None;  // 0x224
    sead::Vector3f mMoveDir;                               // 0x228
    ActorStateRouteDokanMove* mRouteDokanState = nullptr;  // 0x238
    bool mIsSingleMode = false;                            // 0x240
    s32 mDashPanelTime = 0;                                // 0x244
    f32 mDashPanelSpeed = -1.0f;                           // 0x248
    bool mIsShowGuide = false;                             // 0x24c
    s32 mJumpStartTime = 0;                                // 0x250
    sead::Vector3f mBlowDir = sead::Vector3f::zero;        // 0x254
    bool mIsDemoStopped = false;                           // 0x260
    sead::Vector3f mDemoStopVelocity = sead::Vector3f::zero;  // 0x264
};

static_assert(sizeof(Koura) == 0x270);
