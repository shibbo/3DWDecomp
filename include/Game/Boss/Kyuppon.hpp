#pragma once

#include <container/seadObjArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class BlockRailLink;
class BlockRailRider;
class BlockRailRouteSelecter;
class CameraTicket;
class Nerve;
class ParabolicPath;
template <class T>
class DeriveActorGroup;
}  // namespace al

class ActorJointLookController;
class ActorStateRouteDokanMove;
class ActorStateSupportFreeze;
class EnemyEffectBullet;

/**
 * @brief Kyuppon (Hisstocrat, "Prince Pompadour"): a boss that charges at the player, fires
 * bullets and is knocked into a route pipe to be beaten.
 */
class Kyuppon : public al::LiveActor {
public:
    /** @brief Collider and body-sensor sizes at scale 1, used to rescale the boss. */
    struct SizeParam {
        /**
         * @brief Stores the collider and sensor sizes.
         * @param scale Battle scale.
         * @param radius Collider radius.
         * @param offsetY Collider height offset.
         * @param bodyRadius Body sensor radius.
         * @param rBodyOffset Body sensor offset.
         */
        SizeParam(f32 scale, f32 radius, f32 offsetY, f32 bodyRadius,
                  const sead::Vector3f& rBodyOffset)
            : scale(scale), colliderRadius(radius), colliderOffsetY(offsetY),
              sensorRadius(bodyRadius), sensorOffset(rBodyOffset) {}

        f32 scale;
        f32 colliderRadius;
        f32 colliderOffsetY;
        f32 sensorRadius;
        sead::Vector3f sensorOffset;
    };

    using EntranceArray = sead::ObjArray<sead::Vector3f>;

    explicit Kyuppon(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void scaling(f32 scale);
    void appear() override;
    void setNerveLocal(const al::Nerve* pNerve);
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isInvincible() const;
    bool trySlideToEntranceIfSwoon();
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool compareBlockRailRoute(const al::BlockRailRider* pRider, const al::BlockRailLink* pLinkA,
                               const al::BlockRailLink* pLinkB) const;
    void exeDemoAppear();
    void exeRunStart();
    void exeRunStartLoop();
    void exeRun();
    bool tryStartSwoon();
    void updateTargetLost();
    void exeBrake();
    void exeAttackTurn();
    void exeAttackWeapon();
    void exeSlide();
    void exeLost();
    void exeSwoonStart();
    void exeSwoon();
    void exeSwoonShot();
    void exeSwoonEnd();
    void exeRouteDokanMove();
    void exeShoot();
    void exeStun();
    void exeStruggle();
    void resize();
    void exeResize();
    void exeKickBlow();
    void boundCollide();
    void exeKickBlowRecover();
    void exeResizeWeak();
    void exeDead();
    void exeSupportFreeze();

private:
    s32 mLevel = 0;
    SizeParam* mSizeParam = nullptr;
    ActorStateRouteDokanMove* mStateRouteDokanMove = nullptr;
    ActorStateSupportFreeze* mStateSupportFreeze = nullptr;
    al::BlockRailRouteSelecter* mRouteSelecter;
    al::DeriveActorGroup<EnemyEffectBullet>* mBulletGroup = nullptr;
    al::ParabolicPath* mParabolicPath;
    ActorJointLookController* mLookController = nullptr;
    sead::Vector3f mInitTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mInitFront = sead::Vector3f::ez;
    al::LiveActor* mTargetPlayer = nullptr;
    s32 mHitCount = 0;
    sead::Vector3f mSlideDir = {0.0f, 0.0f, 0.0f};
    EntranceArray mEntranceTrans;
    EntranceArray mEntranceFront;
    sead::Matrix34f mSwoonStartMtx = sead::Matrix34f::ident;
    s32 mEntranceIndex = -1;
    s32 mBoundCount = 0;
    s32 mTargetLostCounter = 0;
    bool mIsShootLanded = false;
    bool mIsSingleMode = false;
    al::CameraTicket* mDemoCamera = nullptr;
    bool mIsValidRespawnPos = false;
    sead::Vector3f mRespawnTrans;
    sead::Vector3f mRespawnFront;
    bool mIsValidPlayerStartPos = false;
    sead::Vector3f mPlayerStartTrans;
    sead::Vector3f mPlayerStartFront;
};

static_assert(sizeof(Kyuppon) == 0x270);
