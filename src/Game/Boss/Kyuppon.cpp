#include "Boss/Kyuppon.hpp"

#include <attributes.h>

#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyEffectBullet.hpp"
#include "Layout/IslandMap.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Math/ParabolicPath.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointer.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/ActorStateRouteDokanMove.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Block/BlockRailRider.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
/** @brief Lets the route pipe mover ask Kyuppon which way to go at a fork. */
class KyupponRouteSelecter : public al::BlockRailRouteSelecter {
public:
    /**
     * @brief Creates a route selecter for a Kyuppon.
     * @param pHost Kyuppon that decides the route.
     */
    explicit KyupponRouteSelecter(const Kyuppon* pHost) : mHost(pHost) {}

    /**
     * @brief Forwards the route decision to the host.
     * @param pRider Rail rider at the fork.
     * @param pLinkA First candidate link.
     * @param pLinkB Second candidate link.
     * @return Whether the first link is preferred.
     */
    bool compareBlockRailRoute(const al::BlockRailRider* pRider, const al::BlockRailLink* pLinkA,
                               const al::BlockRailLink* pLinkB) const override {
        return mHost->compareBlockRailRoute(pRider, pLinkA, pLinkB);
    }

private:
    const Kyuppon* mHost;
};

NERVE_DECL(Kyuppon, DemoAppear)
NERVE_DECL(Kyuppon, RouteDokanMove)
NERVE_DECL(Kyuppon, SupportFreeze)

/** @brief Slides while swooned; can be pushed into the route pipe entrance. */
class KyupponNrvSlideToEntrance : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Kyuppon>()->exeSlide();
    }
};

NERVE_DECL(Kyuppon, SwoonShot)
NERVE_DECL(Kyuppon, Resize)
NERVE_DECL(Kyuppon, Slide)
NERVE_DECL(Kyuppon, KickBlow)

/** @brief Slides further after a strong push (shares exeSlide with Slide). */
class KyupponNrvSlideStrong : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Kyuppon>()->exeSlide();
    }
};

NERVE_DECL(Kyuppon, RunStart)
NERVE_DECL(Kyuppon, Run)
NERVE_DECL(Kyuppon, RunStartLoop)

/** @brief Brakes after losing the target, then searches for it (shares exeBrake). */
class KyupponNrvBrakeLost : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<Kyuppon>()->exeBrake();
    }
};

NERVE_DECL(Kyuppon, Brake)
NERVE_DECL(Kyuppon, AttackTurn)
NERVE_DECL(Kyuppon, Lost)
NERVE_DECL(Kyuppon, AttackWeapon)
NERVE_DECL(Kyuppon, Swoon)
NERVE_DECL(Kyuppon, SwoonEnd)
NERVE_DECL(Kyuppon, Shoot)
NERVE_DECL(Kyuppon, Stun)
NERVE_DECL(Kyuppon, Struggle)
NERVE_DECL(Kyuppon, KickBlowRecover)
NERVE_DECL(Kyuppon, Dead)
NERVE_DECL(Kyuppon, ResizeWeak)
NERVE_DECL(Kyuppon, SwoonStart)

NERVES_MAKE_STRUCT(Kyuppon, DemoAppear, RouteDokanMove, SupportFreeze, SlideToEntrance, SwoonShot,
                   Resize, Slide, KickBlow, SlideStrong, RunStart, Run, RunStartLoop, BrakeLost, Brake,
                   AttackTurn, Lost, AttackWeapon, Swoon, Shoot, Stun, Struggle, KickBlowRecover, Dead,
                   ResizeWeak, SwoonStart)
NERVES_MAKE_NOSTRUCT(Kyuppon, SwoonEnd)

/** @brief Number of bullets and the angle between them for one multi-shot volley. */
struct ShotInfo {
    /**
     * @brief Creates a volley description.
     * @param num Number of bullets.
     * @param angle Angle between neighbouring bullets in degrees.
     */
    ShotInfo(s32 num, f32 angle) : num(num), angle(angle) {}

    s32 num;
    f32 angle;
};

/** @brief Spring settings of one hair joint. */
struct HairParam {
    sead::Vector3f childLocalPos;
    f32 stability;
    f32 friction;
    f32 limitDegree;
};

const sead::Vector3f sShotOffset(0.0f, 130.0f, 170.0f);
const ActorStateSupportFreezeParam sSupportFreezeParam(true, 15, false, true, 60,
                                                       sead::Vector3f(0.0f, 300.0f, 150.0f));
const EnemyEffectBulletInfo sBulletInfo("KyupponBullet", 180, "Body", "Appear", "Disappear",
                                        "Fly", "Vanish", nullptr, true, false, 0, 1.0f, 0.1f);
const EnemyEffectBulletInfo sBulletInfoLv2("KyupponBullet", 180, "Body", "Appear", "Disappear",
                                           "Fly", "Vanish", "Lv2", true, true, 15, 1.0f, 0.1f);
const ActorJointLookControllerParam sEyeParam[] = {
    ActorJointLookControllerParam(0.75f, sead::Vector2f(-7.0f, 23.0f), false, nullptr, nullptr),
    ActorJointLookControllerParam(0.75f, sead::Vector2f(-23.0f, 7.0f), false, nullptr, nullptr),
};
const sead::Vector3f sEyeLAxis(0.0f, 1.0f, 0.0f);
const sead::Vector3f sEyeRAxis(0.0f, -1.0f, 0.0f);
const ShotInfo sShotInfo[] = {{3, 50.0f}, {4, 40.0f}, {5, 40.0f}};
const HairParam sHairParam[] = {
    {sead::Vector3f(60.0f, 0.0f, 0.0f), 0.1f, 0.7f, 60.0f},
    {sead::Vector3f(100.0f, 0.0f, 0.0f), 0.1f, 0.7f, 60.0f},
    {sead::Vector3f(100.0f, 0.0f, 0.0f), 0.1f, 0.7f, 60.0f},
};

/**
 * @brief Attaches a spring controller to a hair joint.
 * @param pActor Actor that owns the joint.
 * @param pJointName Hair joint name.
 * @param rParam Spring settings.
 */
inline void initHairSpring(const al::LiveActor* pActor, const char* pJointName,
                           const HairParam& rParam) {
    al::JointSpringController* pController = al::initJointSpringController(pActor, pJointName);
    pController->setChildLocalPos(rParam.childLocalPos);
    pController->setStability(rParam.stability);
    pController->setFriction(rParam.friction);
    pController->setLimitDegree(rParam.limitDegree);
}

/**
 * @brief Checks whether the target is behind or too far to the side to keep charging.
 * @param pActor Charging actor.
 * @param pTarget Chased player.
 * @return Whether the charge should stop.
 */
inline bool isOutOfChaseRange(const al::LiveActor* pActor, const al::LiveActor* pTarget) {
    sead::Vector3f localPos = {0.0f, 0.0f, 0.0f};
    al::multVecInvPose(&localPos, pActor, al::getTrans(pTarget));
    f32 sideDistance = sead::Mathf::abs(localPos.x);
    return localPos.z < -300.0f || sideDistance > 1000.0f;
}

/**
 * @brief Rescales a vector to the given length, leaving a zero vector untouched.
 * @param pVec Vector to rescale.
 * @param length New length.
 */
inline void setLength(sead::Vector3f* pVec, f32 length) {
    f32 current = pVec->length();
    if (current > 0.0f) {
        f32 scale = length / current;
        pVec->x = scale * pVec->x;
        pVec->y = scale * pVec->y;
        pVec->z = scale * pVec->z;
    }
}
}  // namespace

/**
 * @brief Constructs Kyuppon with its route selecter and knock-back path.
 * @param pName Actor name.
 */
Kyuppon::Kyuppon(const char* pName)
    : al::LiveActor(pName), mRouteSelecter(new KyupponRouteSelecter(this)),
      mParabolicPath(new al::ParabolicPath()) {}

/**
 * @brief Initializes the model, states, bullets, demo camera, entrances and joint controllers.
 * @param rInfo Actor placement and scene information.
 */
void Kyuppon::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.getActorSceneInfo().isSingleMode) {
        al::initActorWithArchiveName(this, rInfo, "KyupponFur", nullptr);
    } else {
        al::initActor(this, rInfo);
    }

    al::initNerve(this, &NrvKyuppon.DemoAppear, 2);
    al::invalidateClipping(this);
    al::calcFrontDir(&mInitFront, this);
    al::tryGetArg(&mLevel, rInfo, "Level");

    mStateRouteDokanMove = new ActorStateRouteDokanMove(this, rInfo);
    mStateRouteDokanMove->setRouteSelecter(mRouteSelecter);
    mStateRouteDokanMove->setMoveSpeed(30.0f);
    al::initNerveState(this, mStateRouteDokanMove, &NrvKyuppon.RouteDokanMove, "ルート土管移動状態");

    mStateSupportFreeze = new ActorStateSupportFreeze(this, &sSupportFreezeParam);
    al::initNerveState(this, mStateSupportFreeze, &NrvKyuppon.SupportFreeze, "DRC拘束");

    mSizeParam = new SizeParam(2.25f, al::getColliderRadius(this), al::getColliderOffsetY(this),
                               al::getSensorRadius(this, "Body"),
                               al::getSensorFollowPosOffset(this, "Body"));
    scaling(2.25f);

    static const f32 sColorFrame[] = {0.0f, 1.0f};
    al::startMtpAnimAndSetFrameAndStop(this, "Color", sColorFrame[mLevel]);
    al::startMtpAnimAndSetFrameAndStop(al::getSubActor(this, 0), "Color", sColorFrame[mLevel]);
    al::startMclAnimAndSetFrameAndStop(this, "Color", sColorFrame[mLevel]);
    al::startMclAnimAndSetFrameAndStop(al::getSubActor(this, 0), "Color", sColorFrame[mLevel]);

    const EnemyEffectBulletInfo* pBulletInfo = mLevel == 1 ? &sBulletInfoLv2 : &sBulletInfo;
    mBulletGroup = new al::DeriveActorGroup<EnemyEffectBullet>("キュッポン弾グループ", 15);
    for (s32 i = 0; i < 15; i++) {
        auto* pBullet = new EnemyEffectBullet("キュッポン弾", pBulletInfo);
        al::initCreateActorNoPlacementInfo(pBullet, rInfo);
        mBulletGroup->registerActor(pBullet);
    }

    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) {
        bool isUseCamera = true;
        al::tryGetArg(&isUseCamera, rInfo, "UseCamera");
        if (isUseCamera) {
            mDemoCamera = al::initAnimCamera_RS(this, rInfo, "Anim");
        }

        if (al::calcLinkChildNum(rInfo, "PlayerRestartPos") >= 1) {
            al::PlacementInfo info;
            al::getLinksInfoByIndex(&info, rInfo, "PlayerRestartPos", 0);
            if (al::tryGetTrans(&mRespawnTrans, info) && al::tryGetFront(&mRespawnFront, info)) {
                mIsValidRespawnPos = true;
            }
        }

        if (al::calcLinkChildNum(rInfo, "PlayerStartPos") >= 1) {
            al::PlacementInfo info;
            al::getLinksInfoByIndex(&info, rInfo, "PlayerStartPos", 0);
            if (al::tryGetTrans(&mPlayerStartTrans, info) &&
                al::tryGetFront(&mPlayerStartFront, info)) {
                mIsValidPlayerStartPos = true;
            }
        }
    }

    s32 entranceNum = al::calcLinkChildNum(rInfo, "RouteDokanEntrance");
    mEntranceTrans.allocBuffer(entranceNum, nullptr);
    mEntranceFront.allocBuffer(entranceNum, nullptr);
    for (s32 i = 0; i < entranceNum; i++) {
        al::PlacementInfo info;
        al::getLinksInfoByIndex(&info, al::getPlacementInfo(rInfo), "RouteDokanEntrance", i);
        al::getTrans(mEntranceTrans.emplaceBack(), info);
        al::tryGetFront(mEntranceFront.emplaceBack(), info);
    }

    mLookController = new ActorJointLookController(this, 2);
    al::initJointControllerKeeper(this, mLookController->mParams.capacity() + 3);
    mLookController->appendJoint("EyeL", sEyeLAxis, &sEyeParam[0]);
    mLookController->appendJoint("EyeR", sEyeRAxis, &sEyeParam[1]);
    initHairSpring(this, "Hair1", sHairParam[0]);
    initHairSpring(this, "Hair2", sHairParam[1]);
    initHairSpring(this, "Hair3", sHairParam[2]);

    mInitTrans.set(al::getTrans(this));
    al::setHitSensorMtxPtr(this, "Head", al::getJointMtxPtr(al::getSubActor(this, 0), "Head"));
    al::setHitSensorMtxPtr(this, "BodyUp",
                           al::getJointMtxPtr(al::getSubActor(this, 0), "BodyUp"));
    al::setHitSensorMtxPtr(this, "BodyDown",
                           al::getJointMtxPtr(al::getSubActor(this, 0), "BodyDown"));
    al::trySyncStageSwitchAppear(this);
}

/**
 * @brief Scales the collider and the body sensor relative to their initial size.
 * @param scale Scale factor.
 */
void Kyuppon::scaling(f32 scale) {
    al::setColliderRadius(this, mSizeParam->colliderRadius * scale);
    al::setColliderOffsetY(this, mSizeParam->colliderOffsetY * scale);
    al::setSensorRadius(this, "Body", mSizeParam->sensorRadius * scale);
    al::setSensorFollowPosOffset(this, "Body", mSizeParam->sensorOffset * scale);
}

/** @brief Appears with the entry demo and disables island warping during the battle. */
void Kyuppon::appear() {
    al::LiveActor::appear();
    setNerveLocal(&NrvKyuppon.DemoAppear);
    IslandMap::setIslandWarpEnable(this, false);
}

/**
 * @brief Sets a nerve and toggles the sensors that are only active in some states.
 * @param pNerve Nerve to set.
 */
void Kyuppon::setNerveLocal(const al::Nerve* pNerve) {
    al::setNerve(this, pNerve);

    if (al::isNerve(this, &NrvKyuppon.Stun) || al::isNerve(this, &NrvKyuppon.Struggle)) {
        al::validateHitSensor(this, "Head");
    } else {
        al::invalidateHitSensor(this, "Head");
    }

    if (al::isNerve(this, &NrvKyuppon.KickBlow) || al::isNerve(this, &NrvKyuppon.KickBlowRecover) ||
        al::isNerve(this, &NrvKyuppon.ResizeWeak) || al::isNerve(this, &NrvKyuppon.Dead)) {
        al::validateHitSensor(this, "BodyUp");
        al::validateHitSensor(this, "BodyDown");
    } else {
        al::invalidateHitSensor(this, "BodyUp");
        al::invalidateHitSensor(this, "BodyDown");
    }
}

/** @brief Vanishes together with the shrunken body and re-enables island warping. */
void Kyuppon::kill() {
    IslandMap::setIslandWarpEnable(this, true);
    al::startSe(this, "Vanish");
    al::LiveActor::kill();

    if (al::isAlive(al::getSubActor(this, 0))) {
        al::getSubActor(this, 0)->kill();
    }

    al::tryOnSwitchDeadOn(this);
}

/** @brief Updates the eyes while the shrunken body is hidden and checks death areas. */
void Kyuppon::control() {
    if (al::isDead(al::getSubActor(this, 0))) {
        mLookController->update();
    }

    al::tryKillByDeathArea(this);
}

/**
 * @brief Pushes, hits or enters the route pipe depending on the current state.
 * @param pSelf Sensor of Kyuppon.
 * @param pOther Sensor that was touched.
 */
void Kyuppon::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvKyuppon.DemoAppear)) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) &&
        (al::isNerve(this, &NrvKyuppon.KickBlow) || al::isNerve(this, &NrvKyuppon.KickBlowRecover) ||
         al::isNerve(this, &NrvKyuppon.ResizeWeak) || al::isNerve(this, &NrvKyuppon.Dead))) {
        al::sendMsgPush(pOther, pSelf);
        if (mIsSingleMode) {
            rc::sendMsgPushConnected(pOther, pSelf);
        }
    }

    if (al::isNerve(this, &NrvKyuppon.KickBlow) || al::isNerve(this, &NrvKyuppon.KickBlowRecover) ||
        al::isNerve(this, &NrvKyuppon.ResizeWeak) || al::isNerve(this, &NrvKyuppon.Dead)) {
        return;
    }

    if (al::isNerve(this, &NrvKyuppon.SlideToEntrance)) {
        if (mStateRouteDokanMove->tryStart(pSelf, pOther)) {
            mStateRouteDokanMove->forceCalcMoveDirection();
            al::setVelocityZero(this);
            al::offCollide(this);
            setNerveLocal(&NrvKyuppon.RouteDokanMove);
        }

        return;
    }

    if (al::isNerve(this, &NrvKyuppon.SwoonShot)) {
        if (al::isSensorEnemyBody(pSelf) && al::isSensorPlayer(pOther)) {
            al::sendMsgHitStrong(pOther, pSelf);
        }

        return;
    }

    if (isInvincible()) {
        return;
    }

    if (al::isNerve(this, &NrvKyuppon.Resize)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if ((al::isNerve(this, &NrvKyuppon.SwoonStart) || al::isNerve(this, &NrvKyuppon.Swoon)) &&
        al::isSensorPlayer(pOther) && rc::isPlayerInRouteDokan(al::getSensorHost(pOther))) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorPlayerOrPlayerWeapon(pOther)) {
        if (al::isSensorPlayer(pOther) && rc::isPlayerGiant(pOther)) {
            return;
        }

        if (al::sendMsgHit(pOther, pSelf)) {
            if (al::isSensorPlayer(pOther) && rc::isPlayerHolded(pOther)) {
                return;
            }

            if (trySlideToEntranceIfSwoon()) {
                return;
            }

            al::sendMsgPushStrong(pOther, pSelf);
            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pSelf, pOther);
            mSlideDir = -dir;
            al::startAction(this, "PushFront");
            al::setVelocityToDirection(this, mSlideDir, 15.0f);
            setNerveLocal(al::isNerve(this, &NrvKyuppon.SwoonStart) ||
                                  al::isNerve(this, &NrvKyuppon.Swoon) ?
                              static_cast<const al::Nerve*>(&NrvKyuppon.SlideToEntrance) :
                              &NrvKyuppon.Slide);
            return;
        }
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorKoopaJr(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        if (al::isNerve(this, &NrvKyuppon.SwoonStart) || al::isNerve(this, &NrvKyuppon.Swoon) ||
            al::isNerve(this, &NrvKyuppon.Stun) || al::isNerve(this, &NrvKyuppon.Struggle)) {
            return;
        }

        al::sendMsgEnemyAttack(pOther, pSelf);
        return;
    }

    if (mIsSingleMode && al::isSensorEnemyBody(pSelf) && !al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        rc::sendMsgPushConnected(pOther, pSelf);
    }
}

/**
 * @brief Checks whether Kyuppon currently ignores attacks.
 * @return True while sliding, swooning, shooting out of the pipe, stunned or knocked back.
 */
bool Kyuppon::isInvincible() const {
    if (al::isNerve(this, &NrvKyuppon.SwoonStart) && al::isLessEqualStep(this, 15)) {
        return true;
    }

    if ((al::isNerve(this, &NrvKyuppon.Slide) || al::isNerve(this, &NrvKyuppon.SlideStrong) ||
         al::isNerve(this, &NrvKyuppon.SlideToEntrance)) &&
        al::isLessEqualStep(this, 15)) {
        return true;
    }

    return al::isNerve(this, &NrvKyuppon.SwoonShot) ||
           al::isNerve(this, &NrvKyuppon.RouteDokanMove) || al::isNerve(this, &NrvKyuppon.Shoot) ||
           al::isNerve(this, &NrvKyuppon.Stun) || al::isNerve(this, &NrvKyuppon.Struggle) ||
           al::isNerve(this, &NrvKyuppon.KickBlow) || al::isNerve(this, &NrvKyuppon.KickBlowRecover);
}

/**
 * @brief Slides towards the chosen route pipe entrance when hit while swooning.
 * @return Whether the slide was started.
 */
bool Kyuppon::trySlideToEntranceIfSwoon() {
    if (!al::isNerve(this, &NrvKyuppon.SwoonStart) && !al::isNerve(this, &NrvKyuppon.Swoon) &&
        !al::isNerve(this, &NrvKyuppon.SlideToEntrance)) {
        return false;
    }

    const sead::Vector3f* pEntrance = mEntranceTrans[mEntranceIndex];
    const sead::Vector3f& rTrans = al::getTrans(this);
    mSlideDir.set(pEntrance->x - rTrans.x, 0.0f, pEntrance->z - rTrans.z);
    if (al::normalizeOrZero(&mSlideDir)) {
        al::calcFrontDir(&mSlideDir, this);
        mSlideDir.negate();
    }

    al::setVelocityToDirection(this, mSlideDir, 15.0f);
    setNerveLocal(&NrvKyuppon.SlideToEntrance);
    return true;
}

/**
 * @brief Reacts to player attacks: knock-backs while stunned and slides otherwise.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of Kyuppon.
 * @return Whether the message was handled.
 */
bool Kyuppon::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (mIsSingleMode && al::isNerve(this, &NrvKyuppon.DemoAppear)) {
        return false;
    }

    if ((al::isNerve(this, &NrvKyuppon.SwoonStart) || al::isNerve(this, &NrvKyuppon.Swoon)) &&
        al::isSensorPlayer(pOther) && rc::isPlayerInRouteDokan(al::getSensorHost(pOther))) {
        if (rc::isMsgRouteDokanPlayerReflectNoDamage(pMsg)) {
            al::setNerve(this, &NrvKyuppon.SwoonShot);
            return true;
        }

        return false;
    }

    if (al::isNerve(this, &NrvKyuppon.KickBlow) || al::isNerve(this, &NrvKyuppon.KickBlowRecover) ||
        al::isNerve(this, &NrvKyuppon.ResizeWeak) || al::isNerve(this, &NrvKyuppon.Dead) ||
        al::isNerve(this, &NrvKyuppon.RouteDokanMove)) {
        return false;
    }

    if (mIsSingleMode && al::isMsgKeyThrow(pMsg)) {
        al::sendMsgPush(pOther, pSelf);
    }

    if ((al::isNerve(this, &NrvKyuppon.Stun) || al::isNerve(this, &NrvKyuppon.Struggle)) &&
        al::isSensorEnemyBody(pSelf)) {
        if (al::isMsgPlayerFireBallAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            return true;
        }

        if (al::isMsgPlayerKick(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
            al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
            al::isMsgPlayerClimbRollingAttack(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg) ||
            al::isMsgPlayerRollingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
            al::isMsgPlayerTailAttack(pMsg) ||
            al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
            (mIsSingleMode &&
             (al::isMsgNekoAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
              al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
              al::isMsgKouraThrow(pMsg) || rc::isMsgPackunPush(pMsg) || al::isMsgKeyThrow(pMsg)))) {
            rc::addScore(this, pOther, 100.0f, mHitCount);
            mHitCount++;
            al::startHitReactionBlowHit(this);
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);

            sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            if (al::isNearZero(dir)) {
                al::calcFrontDir(&dir, this);
                dir = -dir;
            }

            setLength(&dir, 40.0f);
            dir.y = 15.0f;
            al::setVelocity(this, dir);
            setNerveLocal(&NrvKyuppon.KickBlow);
            return true;
        }
    }

    if (al::isNerve(this, &NrvKyuppon.Resize) || !al::isSensorEnemyBody(pSelf) || isInvincible()) {
        return false;
    }

    if (al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
        al::isMsgPlayerObjRollingAttackFailure(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
        al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
        al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) ||
        (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
         (al::isMsgNekoAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
          al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg) ||
          al::isMsgKouraThrow(pMsg) || rc::isMsgPackunPush(pMsg) || al::isMsgKeyThrow(pMsg)))) {
        if (al::isMsgPlayerTrample(pMsg) && al::getActorVelocity(pOther).y > 0.0f) {
            return false;
        }

        al::startSe(this, "Trampled");
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        if (trySlideToEntranceIfSwoon()) {
            return true;
        }

        if (al::isSensorPlayer(pOther) &&
            (al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
             al::isMsgPlayerObjRollingAttackFailure(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
             al::isMsgPlayerBodyAttack(pMsg))) {
            mSlideDir.set(rc::getPlayerFront(pOther));
        } else {
            al::calcDirBetweenSensorsH(&mSlideDir, pOther, pSelf);
        }

        if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
            al::startAction(this, "Slide");
        } else {
            al::startAction(this, "PushFront");
        }

        mSlideDir.y = 0.0f;
        if (al::normalizeOrZero(&mSlideDir)) {
            al::calcFrontDir(&mSlideDir, this);
        }

        if (al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
            al::isMsgPlayerObjRollingAttackFailure(pMsg) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
            al::setVelocityToDirection(this, mSlideDir, 20.0f);
        } else {
            al::setVelocityToDirection(this, mSlideDir, 15.0f);
        }

        bool isSlideStrong = al::isMsgPlayerBodyAttackReflect(pMsg) ||
                             al::isMsgPlayerCooperationHipDrop(pMsg) ||
                             al::isMsgPlayerObjRollingAttackFailure(pMsg) ||
                             al::isMsgPlayerObjHipDropReflectAll(pMsg);
        setNerveLocal(isSlideStrong ? static_cast<const al::Nerve*>(&NrvKyuppon.SlideStrong) :
                                      &NrvKyuppon.Slide);
        return true;
    }

    return false;
}

/**
 * @brief Handles touch-screen pokes: freezes Kyuppon or pushes it away from the touch point.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched Kyuppon.
 * @param pTarget Touched screen point target.
 * @return Whether the message was handled.
 */
bool Kyuppon::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                    al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvKyuppon.Stun) || al::isNerve(this, &NrvKyuppon.Struggle) ||
        al::isNerve(this, &NrvKyuppon.KickBlow) || al::isNerve(this, &NrvKyuppon.KickBlowRecover) ||
        al::isNerve(this, &NrvKyuppon.ResizeWeak) || al::isNerve(this, &NrvKyuppon.Dead) ||
        al::isNerve(this, &NrvKyuppon.Resize)) {
        if (al::isMsgTouchAssistTrig(pMsg)) {
            return true;
        }

        return al::isMsgTouchAssist(pMsg);
    }

    if (al::isMsgTouchAssistTrig(pMsg) && !isInvincible()) {
        if (trySlideToEntranceIfSwoon()) {
            return true;
        }

        const sead::Vector3f& rTrans = al::getTrans(this);
        const sead::Vector3f& rHitPos = pPointer->getHitPos();
        mSlideDir.set(rTrans.x - rHitPos.x, 0.0f, rTrans.z - rHitPos.z);
        if (al::normalizeOrZero(&mSlideDir)) {
            al::calcFrontDir(&mSlideDir, this);
        }

        al::setVelocityToDirection(this, mSlideDir, 15.0f);
        setNerveLocal(&NrvKyuppon.Slide);
        al::startAction(this, "PushFront");
        al::startSe(this, "Trampled");
        return true;
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvKyuppon.Slide) && !al::isNerve(this, &NrvKyuppon.SlideStrong) &&
            !al::isNerve(this, &NrvKyuppon.SlideToEntrance) &&
            !al::isNerve(this, &NrvKyuppon.SwoonStart) && !al::isNerve(this, &NrvKyuppon.Swoon) &&
            !isInvincible() && !al::isNerve(this, &NrvKyuppon.SupportFreeze)) {
            al::setVelocityZero(this);
            setNerveLocal(&NrvKyuppon.SupportFreeze);
        }

        return true;
    }

    return false;
}

/**
 * @brief Picks a random branch at route pipe forks.
 * @param pRider Rail rider at the fork.
 * @param pLinkA First candidate link.
 * @param pLinkB Second candidate link.
 * @return Whether the first link is preferred.
 */
bool Kyuppon::compareBlockRailRoute(const al::BlockRailRider* pRider,
                                    const al::BlockRailLink* pLinkA,
                                    const al::BlockRailLink* pLinkB) const {
    return al::isHalfProbability();
}

/** @brief Plays the entry demo, with camera and music in single mode. */
void Kyuppon::exeDemoAppear() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode) {
            rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(3));
            if (mDemoCamera != nullptr) {
                if (!rc::requestStartDemoPlayer(this)) {
                    al::setNerve(this, &NrvKyuppon.DemoAppear);
                    return;
                }

                if (mIsValidPlayerStartPos) {
                    al::LiveActor* pPlayer = rc::findNearestActivePlayerActor(this);
                    rc::addDemoActor(pPlayer);
                    rc::setPlayerTrans(pPlayer, mPlayerStartTrans);
                    rc::setPlayerFrontVec(pPlayer, mPlayerStartFront);
                }

                if (mIsValidRespawnPos) {
                    GameDataHolderAccessor accessor(this);
                    SingleModeDataFunction::setGenericPlayerRespawnPosition(
                        accessor, mRespawnTrans, mRespawnFront);
                    SaveDataAccessFunction::startSaveDataWriteSync(
                        GameDataHolderAccessor(this).getHolder(), true);
                }

                al::startAnimCamera_RS(this, mDemoCamera, "DemoAppearSM", 0);
                rc::addDemoActor(this);
            }

            al::startAction(this, "DemoAppearSM");
            PlayerKoopaJr* pKoopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
            if (pKoopaJr != nullptr) {
                pKoopaJr->startMidBossDemo();
            }
        } else {
            al::startAction(this, "DemoAppear");
        }
    }

    if (mIsSingleMode) {
        al::pauseIslandBgm(this, 60);
        al::pauseOceanBgm(this, -1);
        if (al::isStep(this, 30)) {
            alPadRumbleFunction::startPadRumble(this, "LandStrong", -1, true);
        }
    }

    if (al::isActionEnd(this)) {
        mTargetPlayer = rc::findNearestActivePlayerActor(this);
        if (mIsSingleMode && mDemoCamera != nullptr) {
            al::endCamera_RS(this, mDemoCamera, 60, false);
            rc::requestEndDemoPlayer(this);
        }

        if (mIsSingleMode) {
            al::startBgm(this, "DonketsuPrinceSingleModeBattle", -1, 0, -1, -1);
        }

        setNerveLocal(&NrvKyuppon.RunStart);
    }
}

/** @brief Turns towards the nearest player before running. */
void Kyuppon::exeRunStart() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "RunStart");
        mTargetPlayer = nullptr;
        al::validateHitSensor(this, "Body");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    if (mTargetPlayer == nullptr && rc::calcActivePlayerNum(this) >= 1) {
        mTargetPlayer = rc::findNearestActivePlayerActor(this);
    }

    bool isTurnEnd = mTargetPlayer != nullptr &&
                     al::turnToTarget(this, al::getTrans(mTargetPlayer), 9.0f);
    if (al::isActionEnd(this)) {
        if (isTurnEnd) {
            setNerveLocal(&NrvKyuppon.Run);
        } else {
            setNerveLocal(&NrvKyuppon.RunStartLoop);
        }
    }
}

/** @brief Keeps turning towards the target until it is faced or the start time ran out. */
void Kyuppon::exeRunStartLoop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RunStartLoop");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    if (mTargetPlayer == nullptr && rc::calcActivePlayerNum(this) >= 1) {
        mTargetPlayer = rc::findNearestActivePlayerActor(this);
    }

    if ((mTargetPlayer != nullptr && al::turnToTarget(this, al::getTrans(mTargetPlayer), 9.0f)) ||
        al::isGreaterEqualStep(
            this, sead::Mathi::max(90 - static_cast<s32>(al::getSklAnimFrameMax(this, "RunStart")),
                                   0))) {
        setNerveLocal(&NrvKyuppon.Run);
    }
}

/** @brief Charges forward until the target is passed, lost or the charge time runs out. */
void Kyuppon::exeRun() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Run");
        al::setVelocityZero(this);
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    if (tryStartSwoon()) {
        return;
    }

    updateTargetLost();
    if (mTargetLostCounter >= 60) {
        setNerveLocal(&NrvKyuppon.BrakeLost);
        return;
    }

    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, this);
    al::addVelocityToDirection(this, front, al::calcNerveRate(this, 15) * 0.6f);
    al::addVelocityToGravity(this, 1.0f);
    al::scaleVelocity(this, 0.94f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);

    al::LiveActor* pTarget = mTargetPlayer;
    if (al::isLessEqualStep(this, 30)) {
        return;
    }

    if (al::isGreaterEqualStep(this, 240) || isOutOfChaseRange(this, pTarget)) {
        setNerveLocal(&NrvKyuppon.Brake);
    }
}

/**
 * @brief Starts swooning when a route pipe entrance is close and in front while running.
 * @return Whether the swoon was started.
 */
bool Kyuppon::tryStartSwoon() {
    for (s32 i = 0; i < mEntranceTrans.size(); i++) {
        if (!((al::getTrans(this) - *mEntranceTrans(i)).length() < 400.0f)) {
            continue;
        }

        if (al::isNerve(this, &NrvKyuppon.Run)) {
            sead::Vector3f front = {0.0f, 0.0f, 0.0f};
            al::calcFrontDir(&front, this);
            sead::Vector3f toEntrance = *mEntranceTrans[i] - al::getTrans(this);
            if (sead::Mathf::abs(al::calcAngleOnPlaneDegree(front, toEntrance,
                                                            sead::Vector3f::ey)) > 90.0f) {
                continue;
            }
        }

        mEntranceIndex = i;
        setNerveLocal(&NrvKyuppon.SwoonStart);
        al::setVelocityZero(this);
        return true;
    }

    return false;
}

/** @brief Counts the frames the target player has been bound. */
void Kyuppon::updateTargetLost() {
    if (rc::isPlayerBinded(mTargetPlayer)) {
        mTargetLostCounter++;
    } else {
        mTargetLostCounter = 0;
    }
}

/** @brief Brakes to a stop, then attacks or searches for the lost target. */
void Kyuppon::exeBrake() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Brake");
        mTargetLostCounter = 0;
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    if (tryStartSwoon()) {
        return;
    }

    al::addVelocityToGravity(this, 1.0f);
    al::scaleVelocity(this, 0.94f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    if (al::isActionEnd(this)) {
        al::setVelocityZero(this);
        setNerveLocal(al::isNerve(this, &NrvKyuppon.Brake) ?
                          static_cast<const al::Nerve*>(&NrvKyuppon.AttackTurn) :
                          &NrvKyuppon.Lost);
    }
}

/** @brief Turns towards the target while signalling the attack. */
void Kyuppon::exeAttackTurn() {
    if (mTargetPlayer == nullptr) {
        setNerveLocal(&NrvKyuppon.RunStart);
        return;
    }

    if (al::isFirstStep(this)) {
        al::startAction(this, mLevel == 1 ? "AttackSignLv2" : "AttackSignLv1");
    }

    al::turnToTarget(this, mTargetPlayer, 9.0f);
    mLookController->setLookAtNearestPlayer(-1.0f);
    if (al::isActionEnd(this)) {
        setNerveLocal(&NrvKyuppon.AttackWeapon);
    }
}

/** @brief Fires one bullet, or a fan of bullets at higher levels, then runs again. */
void Kyuppon::exeAttackWeapon() {
    s32 level = mLevel;
    if (al::isFirstStep(this)) {
        al::startAction(this, level == 1 ? "AttackMulti" : "Attack");
    }

    if (!al::isActionPlaying(this, "Wait") && al::isActionEnd(this)) {
        al::startAction(this, "Wait");
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    if (al::isStep(this, level == 1 ? 20 : 4)) {
        if (mLevel == 0) {
            EnemyEffectBullet* pBullet = mBulletGroup->tryFindDeadDeriveActor();
            if (pBullet != nullptr) {
                sead::Vector3f front = {0.0f, 0.0f, 0.0f};
                al::calcFrontDir(&front, this);
                al::calcTransLocalOffset(al::getTransPtr(pBullet), this, sShotOffset);
                pBullet->shot(front, 10.0f);
                al::startSe(this, "Shoot");
            }
        } else {
            const ShotInfo& rShotInfo = sShotInfo[mHitCount];
            sead::Vector3f front = {0.0f, 0.0f, 0.0f};
            al::calcFrontDir(&front, this);
            for (s32 i = 0; i < rShotInfo.num; i++) {
                EnemyEffectBullet* pBullet = mBulletGroup->tryFindDeadDeriveActor();
                if (pBullet == nullptr) {
                    continue;
                }

                sead::Vector3f dir = front;
                al::rotateVectorDegreeY(&dir, rShotInfo.angle * (i + (rShotInfo.num - 1) * -0.5f));
                al::calcTransLocalOffset(al::getTransPtr(pBullet), this, sShotOffset);
                pBullet->shot(dir, 10.0f);
                al::startSe(this, "Shoot");
            }
        }
    }

    updateTargetLost();
    if (al::isGreaterEqualStep(this, 90)) {
        if (mTargetLostCounter >= 60) {
            setNerveLocal(&NrvKyuppon.Lost);
        } else {
            setNerveLocal(&NrvKyuppon.RunStart);
        }
    }
}

/** @brief Slides along the push direction, then turns to attack. */
void Kyuppon::exeSlide() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "スライド");
        mLookController->stopLook();
        al::faceToDirection(this, -mSlideDir);
        if (al::isNoCollide(this)) {
            al::onCollide(this);
        }
    }

    if (!al::isNerve(this, &NrvKyuppon.SlideToEntrance) && tryStartSwoon()) {
        return;
    }

    al::addVelocityToDirection(
        this, mSlideDir,
        al::calcNerveEaseInOutValue(this, 0, 25, al::isNerve(this, &NrvKyuppon.SlideStrong) ? 1.5f : 1.0f,
                                    0.0f));
    al::scaleVelocity(this, 0.94f);
    if (al::isGreaterEqualStep(this, 40)) {
        al::setVelocityZero(this);
        setNerveLocal(&NrvKyuppon.AttackTurn);
        return;
    }

    al::holdSe(this, "Pushed");
}

/** @brief Looks around until a free player is found again. */
void Kyuppon::exeLost() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Lost");
        mLookController->stopLook();
    }

    if (al::isGreaterEqualStep(this, 15)) {
        sead::Vector3f trans = al::getTrans(this);
        al::LiveActor* pNearest = nullptr;
        f32 minDistance = sead::Mathf::maxNumber();
        for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
            al::LiveActor* pPlayer = al::getPlayerActor(this, i);
            if (pPlayer == nullptr || rc::isPlayerDeadOrBubble(pPlayer) ||
                rc::isPlayerBinded(pPlayer)) {
                continue;
            }

            f32 distance = (al::getTrans(pPlayer) - trans).length();
            if (distance < minDistance) {
                minDistance = distance;
                pNearest = pPlayer;
            }
        }

        if (pNearest != nullptr) {
            mTargetPlayer = pNearest;
            setNerveLocal(&NrvKyuppon.RunStart);
        }
    }
}

/** @brief Blends into position in front of the chosen route pipe entrance and swoons. */
void Kyuppon::exeSwoonStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwoonStart");
        mLookController->stopLook();
        mSwoonStartMtx = *getBaseMtx();
        al::offCollide(this);
    }

    if (al::isLessEqualStep(this, 5)) {
        sead::Matrix34f targetMtx = sead::Matrix34f::ident;
        sead::Matrix34f mtx = sead::Matrix34f::ident;
        sead::Vector3f pos = *mEntranceTrans[mEntranceIndex];
        const sead::Vector3f& rFront = *mEntranceFront(mEntranceIndex);
        pos.set(rFront.x * 240.0f + pos.x, mSwoonStartMtx(1, 3), rFront.z * 240.0f + pos.z);
        al::makeMtxFrontUpPos(&targetMtx, *mEntranceFront[mEntranceIndex], sead::Vector3f::ey,
                              pos);
        al::blendMtx(&mtx, mSwoonStartMtx, targetMtx, al::calcNerveRate(this, 5));
        al::updatePoseMtx(this, &mtx);
    }

    if (al::isActionEnd(this)) {
        setNerveLocal(&NrvKyuppon.Swoon);
    }
}

/** @brief Stays dizzy in front of the route pipe for a while. */
void Kyuppon::exeSwoon() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwoonLoop");
        al::setVelocityZero(this);
    }

    mLookController->setLookAtNearestPlayer(-1.0f);
    if (al::isGreaterEqualStep(this, 120)) {
        setNerveLocal(&NrvKyuppon.SwoonShot);
    }
}

/** @brief Shoots forward out of the swoon. */
void Kyuppon::exeSwoonShot() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwoonShot");
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&front, this);
        al::setVelocityToDirection(this, front, 20.0f);
    }

    al::scaleVelocity(this, 0.94f);
    if (al::isGreaterEqualStep(this, 14)) {
        setNerveLocal(&NrvKyupponSwoonEnd);
        al::setVelocityZero(this);
    }
}

/** @brief Recovers from the swoon. */
void Kyuppon::exeSwoonEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwoonEnd");
        al::onCollide(this);
    }

    if (al::isActionEnd(this)) {
        setNerveLocal(&NrvKyuppon.RunStart);
    }
}

/** @brief Shrinks into the route pipe and travels through it. */
void Kyuppon::exeRouteDokanMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RouteDokanMove");
        al::startSe(this, "RouteDokanEnter");
        al::startSe(this, "RouteDokanMove");
        al::startHitReaction(this, "ルート土管イン");
    }

    if (al::isLessEqualStep(this, 8)) {
        f32 scale = al::calcNerveValue(this, 8, mSizeParam->scale, 1.0f);
        al::setColliderRadius(this, scale * mSizeParam->colliderRadius);
        al::setColliderOffsetY(this, scale * mSizeParam->colliderOffsetY);
        al::setSensorRadius(this, "Body", scale * mSizeParam->sensorRadius);
        const sead::Vector3f& rOffset = mSizeParam->sensorOffset;
        al::setSensorFollowPosOffset(
            this, "Body", {scale * rOffset.x, scale * rOffset.y, scale * rOffset.z});
    }

    if (al::isStep(this, 8)) {
        al::getSubActor(this, 0)->appear();
        al::startAction(al::getSubActor(this, 0), "RouteDokanMove");
        mLookController->resetRotate(true);
    }

    bool isEnd = al::updateNerveState(this);
    al::turnToDirection(this, mStateRouteDokanMove->getMoveDirection(), 180.0f);
    if (isEnd) {
        setNerveLocal(&NrvKyuppon.Shoot);
        al::onCollide(this);
        al::stopSeByName(this, "RouteDokanMove");
        al::startSe(this, "RouteDokanExit");
        al::startHitReaction(this, "ルート土管アウト");
    }
}

/** @brief Flies out of the route pipe and skids to a stop. */
void Kyuppon::exeShoot() {
    if (al::isFirstStep(this)) {
        al::startAction(al::getSubActor(this, 0), "Weak");
        al::scaleVelocity(this, 0.5f);
        al::invalidateHitSensor(this, "Body");
        mIsShootLanded = false;
    }

    if (al::isCollidedGround(this)) {
        const sead::Vector3f& rVelocity = al::getVelocity(this);
        sead::Vector3f velocityH = {rVelocity.x, 0.0f, rVelocity.z};
        f32 speed = al::converge(velocityH.length(), 0.0f, 2.0f);
        f32 length = velocityH.length();
        if (length > 0.0f) {
            velocityH *= speed / length;
        }

        al::setVelocity(this, velocityH + sead::Vector3f(0.0f, al::getVelocity(this).y, 0.0f));
        if (!mIsShootLanded) {
            mIsShootLanded = true;
            al::startHitReaction(this, "ルート土管発射着地");
        }
    }

    al::addVelocityToGravity(this, 1.0f);
    const sead::Vector3f& rVelocity = al::getVelocity(this);
    if (al::isNearZero(sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z))) {
        setNerveLocal(&NrvKyuppon.Stun);
        al::setVelocityZero(this);
    }
}

/** @brief Lies weakened on the ground for a moment. */
void Kyuppon::exeStun() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "Weak");
    }

    if (al::isGreaterEqualStep(this, 30)) {
        setNerveLocal(&NrvKyuppon.Struggle);
    }
}

/** @brief Struggles while weakened until Kyuppon recovers its size. */
void Kyuppon::exeStruggle() {
    if (al::isGreaterEqualStep(this, 120)) {
        al::stopSeByName(this, "Weak");
        al::startSe(this, "Recover");
        resize();
    }
}

/** @brief Hides the shrunken body and grows back to battle size. */
void Kyuppon::resize() {
    al::getSubActor(this, 0)->kill();
    al::validateHitSensor(this, "Body");
    al::startAction(this, "Resize");
    scaling(2.25f);
    setNerveLocal(&NrvKyuppon.Resize);
}

/** @brief Waits for the grow animation to end. */
void Kyuppon::exeResize() {
    if (al::isActionEnd(this)) {
        setNerveLocal(&NrvKyuppon.RunStart);
    }
}

/** @brief Flies after being kicked, bouncing off the ground, ceiling and walls. */
void Kyuppon::exeKickBlow() {
    if (al::isFirstStep(this)) {
        al::startAction(al::getSubActor(this, 0), "Damage");
        al::startHitReaction(this, "キック");
        al::stopSeByName(this, "Weak");
        mBoundCount = 0;
    }

    if ((al::isCollidedGround(this) && al::getVelocity(this).y < 0.0f) ||
        (al::isCollidedCeilingVelocity(this) && al::getVelocity(this).y > 0.0f)) {
        sead::Vector3f* pVelocity = al::getVelocityPtr(this);
        pVelocity->y = -pVelocity->y;
        boundCollide();
    }

    if (al::isCollidedWallVelocity(this)) {
        sead::Vector3f normal = al::getCollidedWallNormal(this);
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        sead::Vector3f velocity = al::getVelocity(this);
        velocity.y = 0.0f;
        al::calcFrontDir(&front, this);
        if (al::isReverseDirection(normal, front, 0.0f)) {
            al::turnDirectionDegree(this, &normal, front, 30.0f);
        } else if (!al::isReverseDirection(normal, front, 0.8f)) {
            al::turnDirectionDegree(this, &normal, -front, 20.0f);
        }

        al::calcReflectionVector(&velocity, normal, 1.0f, 0.0f);
        al::normalizeOrDirZ(&front, velocity);
        al::faceToDirection(this, front);
        velocity.y = al::getVelocity(this).y;
        al::setVelocity(this, velocity);
        boundCollide();
    }

    if (mBoundCount >= 1) {
        setNerveLocal(&NrvKyuppon.KickBlowRecover);
    }
}

/** @brief Plays the bounce reaction and counts the bounce. */
void Kyuppon::boundCollide() {
    al::startHitReaction(this, "壁反射");
    mBoundCount++;
}

/** @brief Jumps back to the initial position after a kick, or dies after the third hit. */
void Kyuppon::exeKickBlowRecover() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        mParabolicPath->initFromUpVectorAddHeight(al::getTrans(this), mInitTrans,
                                                  sead::Vector3f::ey, 500.0f);
        const sead::Vector3f& rTrans = al::getTrans(this);
        sead::Vector3f dir = {mInitTrans.x - rTrans.x, 0.0f, mInitTrans.z - rTrans.z};
        al::normalizeOrDirZ(&dir);
        al::faceToDirection(this, dir);
        if (mHitCount >= 3 && mIsSingleMode) {
            al::stopBgm(this, "DonketsuPrinceSingleModeBattle", 20, -1);
        }
    }

    if (mIsSingleMode && mHitCount >= 3) {
        al::pauseIslandBgm(this, -1);
        al::pauseOceanBgm(this, -1);
    }

    mParabolicPath->calcPosition(al::getTransPtr(this), al::calcNerveRate(this, 60));
    if (al::isGreaterEqualStep(this, 60)) {
        if (mHitCount >= 3) {
            setNerveLocal(&NrvKyuppon.Dead);
        } else {
            al::startHitReaction(this, "着地");
            setNerveLocal(&NrvKyuppon.ResizeWeak);
        }
    }
}

/** @brief Shows the weakened body before growing back. */
void Kyuppon::exeResizeWeak() {
    if (al::isFirstStep(this)) {
        al::startAction(al::getSubActor(this, 0), "ResizeWeak");
        al::faceToDirection(this, mInitFront);
    }

    if (al::isActionEnd(al::getSubActor(this, 0))) {
        resize();
    }
}

/** @brief Plays the defeat animation and the after-battle music in single mode. */
void Kyuppon::exeDead() {
    if (al::isFirstStep(this)) {
        al::startAction(al::getSubActor(this, 0), "Dead");
        al::faceToDirection(this, mInitFront);
    }

    if (mIsSingleMode) {
        al::pauseIslandBgm(this, -1);
        al::pauseOceanBgm(this, -1);
    }

    if (al::isActionEnd(al::getSubActor(this, 0))) {
        if (mIsSingleMode) {
            DisasterModeController* pController = DisasterModeController::tryGetController(this);
            al::BgmPlayingRequest request("AfterBattleSingleMode");
            if (pController != nullptr && pController->isDisasterMode()) {
                request._18 = 163300;
            }

            al::startBgm(this, request);
            al::resumeIslandBgm(this, -1);
            al::resumeOceanBgm(this, -1);
        }

        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Stays frozen by the touch screen until the support-freeze state ends. */
void Kyuppon::exeSupportFreeze() {
    if (al::updateNerveStateAndNextNerve(this, &NrvKyuppon.RunStart)) {
        al::onCollide(this);
    }
}
