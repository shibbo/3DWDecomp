#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "NPC/IUseNekoModeActor.hpp"
#include "NPC/Neko.hpp"

class ActorStateSupportFreeze;
class NpcStateChase;
class NpcStateChaseParam;
class NpcStateWander;
class NpcStateWanderParam;
class NpcTargetFinder;
class WalkerStateJump;

/**
 * @brief Placement parameters of a disaster cat (Bowser's Fury cat that attacks the player).
 * @note Shares its first fields with neko::Param.
 */
struct NekoDisasterParam {
    /**
     * @brief Behavior a disaster cat starts with.
     * @note The values have not been reconstructed yet.
     */
    enum StartBehavior : s32 {};

    const char* mComment = nullptr;
    bool mIsDisabledPR = false;
    bool mIsDisablePlessieChase = false;
    f32 mChaseRange = -1.0f;
    bool mIsEnableCliffCheck = true;
    bool mIsEnableShoreCheck = true;
    StartBehavior mStartBehavior = {};
    f32 mWanderRange = 500.0f;
};

static_assert(sizeof(NekoDisasterParam) == 0x20);

/**
 * @brief Mode actor of a disaster cat that wanders around its placement and attacks the player.
 */
class NekoDisaster : public IUseNekoModeActor {
public:
    NekoDisaster(Neko* pHost);

    void init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
              NpcTargetFinder* pTargetFinder) override;
    bool tryStartDefaultBehavior();
    void control() override;
    bool isInteractive() const;
    bool isAttack() const;
    void startClipped() override;
    void endClipped() override;
    void startAttach(const NekoAttachReason& rReason) override;
    void startKill(bool isDeleteParticle) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isEnableAttack() const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool canFreeze() const;
    void exeWait();
    bool tryStartReactToTarget();
    void exeWander();
    void exeSeekPlacementPosition();
    void exeFall();
    void exeStartle();
    void exeFindFace();
    void exeFind();
    void exeFindEnd();
    void exeChase();
    void exeChaseEnd();
    void exeHitReact();
    void exeSupportFreeze();
    void exeStun();
    void exeStunEnd();
    void exeAttack();
    void exeRunAway();
    void exeRunAwayEnd();
    bool isRunAway() const;
    bool isChase() const;
    bool isWait() const;
    bool isStun() const;

    /** @brief Does nothing, a disaster cat has no anticipation. */
    void startDisasterAnticipation(bool isEmitEffect) override {}

    /** @return Always false, a disaster cat has no disaster demo. */
    bool startDisasterDemo() override { return false; }

    /** @return Always false, a disaster cat never seeks a target. */
    bool startSeekTarget(const neko::Target* pTarget, bool isForce) override { return false; }

    /** @brief Does nothing, a disaster cat has no links to appear. */
    void startAppearLinks() override {}

    /** @return Always false, a disaster cat cannot be held. */
    bool isHold() const override { return false; }

    /** @return Always false, a disaster cat cannot be ridden. */
    bool isRide() const override { return false; }

    /** @return Unique id of the host cat. */
    s32 getUID() const override { return mHost->getUID(); }

    /** @return Coat color of the cat. */
    neko::ColorType getNekoType() const override { return mColorType; }

    /** @return Always -1, a disaster cat uses the chase range of its placement parameters. */
    f32 getChaseRange() const override { return -1.0f; }

    /** @return Placement parameters of the cat. */
    const neko::Param* getParam() const override {
        return reinterpret_cast<const neko::Param*>(mParam);
    }

    /** @return Placement parameters of the cat. */
    const NekoDisasterParam* getDisasterParam() const { return mParam; }

    /** @brief Does nothing, a disaster cat cannot hide. */
    void onStartHide() override {}

    /** @return Always true, a disaster cat accepts every target. */
    bool acceptTarget(const al::LiveActor* pTarget,
                      const npc::NpcFindTargetType& rType) const override {
        return true;
    }

private:
    Neko* mHost = nullptr;
    NekoDisasterParam* mParam = nullptr;
    neko::ColorType mColorType = static_cast<neko::ColorType>(6);
    NpcStateWander* mStateWander = nullptr;
    NpcStateChase* mStateChase = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    WalkerStateJump* mStateAttack = nullptr;
    NpcTargetFinder* mTargetFinder = nullptr;
    NpcStateWanderParam* mWanderParam = nullptr;
    NpcStateChaseParam* mChaseParam = nullptr;
    al::HitSensor* mControlSensor = nullptr;
    sead::Vector3f mHitReactVelocity = sead::Vector3f::zero;
    s32 mHitReactCoolTime = 0;
    al::HitSensor* mRunAwaySensor = nullptr;
    s32 mReactWaitTime = -1;
    s32 mPackunEatCoolTime = 0;
};

static_assert(sizeof(NekoDisaster) == 0x1d0);
