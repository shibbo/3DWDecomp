#pragma once

#include <attributes.h>
#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "NPC/IUseNekoModeActor.hpp"
#include "NPC/Neko.hpp"

namespace al {
class ActorCollisionController;
}  // namespace al

class ActorStateSupportStroke;
class ItemStatePlayerHold;
class NekoBindPuppeteer;
class NekoParent;
class NpcHeadController;
class NpcStateChase;
class NpcStateChaseParam;
class NpcStateRunAway;
class NpcStateRunAwayParam;
class NpcStateWander;
class NpcStateWanderParam;
class SePlayObj;

namespace al {
namespace param {

/**
 * @brief Idle behaviors of a regular cat.
 */
enum NekoBehavior : s32 {
    NekoBehavior_Random = 0,  ///< Randomly chain the idle behaviors.
    NekoBehavior_Sit = 2,
    NekoBehavior_Wait = 3,
    NekoBehavior_Play = 4,
    NekoBehavior_LayDown = 5,
    NekoBehavior_Sleep = 6,
    NekoBehavior_Goal = 7,     ///< Wait at the placement position, the cat is already home.
    NekoBehavior_Default = 8,  ///< Use the behavior of the placement parameters.
};

}  // namespace param
}  // namespace al

/**
 * @brief Placement parameters of a regular cat.
 * @note Shares its first fields with neko::Param.
 */
struct NekoNormalParam {
    const char* mComment = nullptr;
    bool mIsDisabledPR = false;
    bool mIsDisablePlessieChase = false;
    f32 mChaseRange = -1.0f;
    bool mIsEnableCliffCheck = true;
    bool mIsEnableShoreCheck = true;
    al::param::NekoBehavior mStartBehavior = al::param::NekoBehavior_Random;
    f32 mWanderRange = 500.0f;
};

static_assert(sizeof(NekoNormalParam) == 0x20);

/**
 * @brief Mode actor of a regular cat that wanders around and can be carried to its parent.
 */
class NekoNormal : public IUseNekoModeActor {
public:
    NekoNormal(Neko* pHost);

    void init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
              NpcTargetFinder* pTargetFinder) override;
    bool tryStartDefaultBehavior(al::param::NekoBehavior behavior);
    void control() override;
    bool isThrown() const;
    bool tryBounce(f32 speed);
    bool isRunAway() const;
    void calcAnim() override;
    void movementPaused(bool isPaused) override;
    void startClipped() override;
    void endClipped() override;
    void updateCollider() override;
    void startAttach(const NekoAttachReason& rReason) override;
    void startKill(bool isDeleteParticle) override;
    void endHold();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack() const;
    bool isChase() const;
    bool tryStartReactToTarget();
    bool tryStartHitReact(const al::HitSensor* pSelf, const al::HitSensor* pOther,
                          const al::SensorMsg* pMsg);
    bool isFollow() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool isEnableHold() const;
    void releaseDropBack();
    void releaseThrow();
    void releaseDrop();
    void releaseDamage(const al::HitSensor* pSelf, const al::HitSensor* pOther);
    void releaseRide();
    bool isWait() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableGoal() const;
    void receivedBindCancel(NekoBindPuppeteer* pPuppeteer, al::HitSensor* pSensor);
    void exeWaitStart();
    void exeWait();
    void exeWaitEnd();
    void exeWaitPush();
    void exeWander();
    void exeFall();
    void exeStartle();
    bool isNearPlayer(f32 range) const;
    void exeStartleWait();
    void exeHitReact();
    void exeTrampled();
    void exeRunAway();
    void exeRunAwayFast();
    void exeRunAwayAvoid();
    void exeRunAwayWait();
    void exeRunAwayEnd();
    void exeChase();
    void exeChaseEnd();
    void exeAlert();
    void exeAlertEnd();
    void exeFaceTarget();
    void exeFollowStart();
    void exeFollow();
    void exeFollowWait();
    void exeFollowJump();
    void exeFollowJumpToPlayer();
    void exeFollowJumpEnd();
    void exeStroke();
    void exePurr();
    void exeBindStart();
    void exeBindEnd();
    void exeBindRide();
    void exeBindRideEnd();
    void exeHold();
    void startHold();
    void exeRelease();
    void exeAttack();
    void exeTargetWait();
    void exePounce();
    void exeAppear();
    void exeSeekTarget();
    void exeGoalWait();
    bool startSeekTarget(const neko::Target* pTarget, bool isForce) override;
    bool isAtGoal() const;
    virtual bool isInteractive() const;
    bool isHold() const override;
    bool isRide() const override;
    bool canCollect() const;
    void startRelease(sead::Vector3f dir, f32 speedH, f32 speedV);
    void releaseDropFront();
    void startDisasterAnticipation(bool isEmitEffect) override;
    bool startDisasterDemo() override;
    void setNekoParent(const NekoParent* pParent);
    void startBindNpc(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void endBindNpc(NpcPuppetBindEndType type) override;
    void setTransVec(const sead::Vector3f& rTrans) override;
    const sead::Vector3f& getTransVec() const override;
    void setUpVec(const sead::Vector3f& rUp) override;
    const sead::Vector3f& getUpVec() const override;
    void setFrontVec(const sead::Vector3f& rFront) override;
    const sead::Vector3f& getFrontVec() const override;
    void setMtx(const sead::Matrix34f* pMtx) override;
    void onStartHide() override;

    /** @brief Does nothing, a regular cat has no links to appear. */
    void startAppearLinks() override {}

    /** @return Unique id of the host cat. */
    s32 getUID() const override { return mHost->getUID(); }

    /** @return Coat color of the cat. */
    neko::ColorType getNekoType() const override { return mColorType; }

    /** @return Range within which the cat reacts to the player. */
    f32 getChaseRange() const override { return mChaseRange; }

    /** @return Placement parameters of the cat. */
    const neko::Param* getParam() const override {
        return reinterpret_cast<const neko::Param*>(mParam);
    }

    /**
     * @brief Store the stick input of the player riding the cat.
     * @param x Horizontal stick input.
     * @param y Vertical stick input.
     */
    void setPlayerPuppetInputTurnStick(f32 x, f32 y) override {
        mPuppetInputStick.x = x;
        mPuppetInputStick.y = y;
    }

    /** @return Always true, a regular cat accepts every target. */
    bool acceptTarget(const al::LiveActor* pTarget,
                      const npc::NpcFindTargetType& rType) const override {
        return true;
    }

    /** @return Host cat of the mode. */
    Neko* getHost() const { return mHost; }

private:
    ALWAYS_INLINE void updatePassiveMovement();
    ALWAYS_INLINE bool tryLimitMove();
    ALWAYS_INLINE bool isLimitMoveNext();
    ALWAYS_INLINE bool isFollowPlayerNear() const;
    ALWAYS_INLINE void stopCollectSe();
    ALWAYS_INLINE void lookAtRunAwayDir(bool isFast);
    ALWAYS_INLINE void lookAwayFrom(const sead::Vector3f& rPos);

    Neko* mHost;                                                    // 0x158
    neko::ColorType mColorType;                                     // 0x160
    NekoNormalParam* mParam;                                        // 0x168
    al::param::NekoBehavior mBehavior = al::param::NekoBehavior_Default;  // 0x170
    NpcStateWander* mStateWander = nullptr;                         // 0x178
    NpcStateChase* mStateChase = nullptr;                           // 0x180
    NpcStateRunAway* mStateRunAway = nullptr;                       // 0x188
    NpcStateRunAway* mStateRunAwayFast = nullptr;                   // 0x190
    ActorStateSupportStroke* mStateSupportStroke = nullptr;         // 0x198
    NpcTargetFinder* mTargetFinder = nullptr;                       // 0x1a0
    NpcStateWanderParam* mWanderParam = nullptr;                    // 0x1a8
    NpcStateChaseParam* mChaseParam = nullptr;                      // 0x1b0
    NpcStateRunAwayParam* mRunAwayParam = nullptr;                  // 0x1b8
    NpcStateRunAwayParam* mRunAwayFastParam = nullptr;              // 0x1c0
    al::ActorCollisionController* mCollisionController = nullptr;   // 0x1c8
    NekoBindPuppeteer* mBindPuppeteer = nullptr;                    // 0x1d0
    NekoBindPuppeteer** mBindPuppeteers = nullptr;                  // 0x1d8
    ItemStatePlayerHold* mStatePlayerHold = nullptr;                // 0x1e0
    al::HitSensor* mHolderSensor = nullptr;                         // 0x1e8
    al::HitSensor* mRideSensor = nullptr;                           // 0x1f0
    NpcPuppetBindEndType mBindEndType;                              // 0x1f8
    sead::Vector2f mPuppetInputStick = sead::Vector2f::zero;        // 0x1fc
    sead::Matrix34f mPuppetMtx = sead::Matrix34f::ident;            // 0x204
    bool mIsValidPuppetMtx = false;                                 // 0x234
    sead::Vector3f mHitReactVelocity = sead::Vector3f::zero;        // 0x238
    f32 mChaseRange = -1.0f;                                        // 0x244
    f32 mReactCoolTime = 0.0f;                                      // 0x248
    sead::Vector3f mLookAtPos;                                      // 0x24c
    al::HitSensor* mAvoidSensor = nullptr;                          // 0x258
    s32 mCoolTime = 0;                                              // 0x260
    s32 mBehaviorTime = 0;                                          // 0x264
    s32 mActionTime = 0;                                            // 0x268
    s32 mPackunEatCoolTime = 0;                                     // 0x26c
    bool mIsOnGround = false;                                       // 0x270
    bool mIsAtGoal = false;                                         // 0x271
    neko::Target* mGoalTarget = nullptr;                            // 0x278
    NpcHeadController* mHeadController = nullptr;                   // 0x280
    al::LiveActor* mFollowPlayer = nullptr;                         // 0x288
    f32 mFollowSpeed = 0.0f;                                        // 0x290
    s32 mRunAwayTime = 0;                                           // 0x294
    bool mIsPushed = false;                                         // 0x298
    SePlayObj* mCollectSe = nullptr;                                // 0x2a0
};

static_assert(sizeof(NekoNormal) == 0x2a8);
