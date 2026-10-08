#include "MapObj/Shards.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Item/AcquireItemFunc.hpp"
#include "Library/LiveActor/LiveActorFlag.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/ActorStateDemoCamera.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/ItemAssistRotateParam.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/ShardsWatcher.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(Shards, Appear);
    NERVE_DECL(Shards, SpinDrc);
    NERVE_DECL(Shards, PopUpFront);
    NERVE_DECL(Shards, DemoAppear);
    NERVE_DECL(Shards, Attached);
    NERVE_DECL(Shards, DirectAppear);
    NERVE_DECL(Shards, Wait);
    NERVE_DECL(Shards, Disappear);
    NERVE_DECL(Shards, Got);
    NERVE_DECL(Shards, DirectDisappear);
    NERVE_DECL(Shards, GotLast);
    NERVES_MAKE_NOSTRUCT(Shards, Appear, SpinDrc, PopUpFront, DemoAppear, Attached, DirectAppear,
                         Wait, Disappear, Got, DirectDisappear, GotLast)

    const ItemAssistRotateParam sShardsAssistRotateParam(60, 12.0f, true, 3.0f, 120);
}  // namespace

/**
 * @brief Construct a shard.
 * @param pName The actor name.
 * @param isAttach Whether the shard starts attached to its host (e.g. inside a bubble).
 */
Shards::Shards(const char* pName, bool isAttach) : al::LiveActor(pName), mIsAttach(isAttach) {}

/**
 * @brief Invalidate the clipping of the shard and of its "already collected" model.
 */
inline void Shards::invalidateClippingAll() {
    al::invalidateClipping(this);

    if (mEmptyActor != nullptr) {
        al::invalidateClipping(mEmptyActor);
    }
}

/**
 * @brief Validate the clipping of the shard and of its "already collected" model.
 */
inline void Shards::validateClippingAll() {
    al::validateClipping(this);

    if (mEmptyActor != nullptr) {
        al::validateClipping(mEmptyActor);
    }
}

/**
 * @brief Stop the idle effect while the shard is clipped during a demo, and restart it after.
 */
inline void Shards::updateDemoEffect() {
    if (mIsClippedInDemo) {
        mIsDemoEffectStopped = true;

        if (al::isActionPlaying(this, "Wait")) {
            al::tryDeleteEffectAndParticle(this, "Wait");
        }
    } else if (mIsDemoEffectStopped) {
        al::tryEmitEffect(this, "Wait", nullptr);
        mIsDemoEffectStopped = false;
    }
}

/**
 * @brief Move the shard up and down along its appear arc and spin it.
 * @param rate The eased appear progress, from 0 to 1.
 */
inline void Shards::updateAppearMove(f32 rate) {
    f32 height = sead::Mathf::sin(sead::Mathf::deg2rad(rate * 180.0f)) * 200.0f;
    al::setTrans(this, sead::Vector3f(0.0f, height, 0.0f) + mAppearTrans);
    rotate(al::lerpValue(rate, 20.0f, 3.0f));
}

/**
 * @brief Initialize the shard from its placement.
 * @param rInfo The actor init info.
 */
void Shards::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::tryGetArg(&mShardId, rInfo, "ShardID");
    al::tryGetArg(&mUsingOccludedEffect, rInfo, "UsingOccludedEffect");
    al::tryGetArg(&mIgnoreIslandOffset, rInfo, "IgnoreIslandOffset");
    al::tryGetArg(&mDisconnectWhenGot, rInfo, "DisconnectWhenGot");
    mIslandId = mPlacementHolder->getZoneNo();

    if (mIslandId < 0) {
        return;
    }

    al::tryGetArg(&mUseFrontAngle, rInfo, "IsForceFrontDir");

    if (mUseFrontAngle) {
        al::tryGetArg(&mFrontAngle, rInfo, "ForcedFrontAngle");
    }

    sharedInit(rInfo);
}

/**
 * @brief Initialization shared by placed shards and shards created by a ShardsWatcher.
 * @param rInfo The actor init info.
 */
void Shards::sharedInit(const al::ActorInitInfo& rInfo) {
    if (mIsInitialized) {
        return;
    }

    bool isCollected = false;

    if (mIslandId > 0) {
        isCollected = SingleModeDataFunction::isShardCollected(GameDataHolderAccessor(this),
                                                               mIslandId - 1, mShardId - 1);
    }

    bool isAttach = mIsAttach;
    al::initNerve(this, &NrvShardsAppear, 3);
    alPlacementFunction::tryGetModelName(&mModelName, rInfo);
    mModelName = mModelName != nullptr ? mModelName : "CollectItem";
    const char* suffix = isAttach ? "Attach" : nullptr;
    al::initActorWithArchiveName(this, rInfo, mModelName, suffix);
    mCollected = isCollected;

    mEmptyActor = new al::LiveActor("CollectItemEmpty");
    al::initActorWithArchiveName(mEmptyActor, rInfo, "CollectItemEmpty", suffix);
    mEmptyActor->makeActorAppeared();

    if (mCollected) {
        al::hideModelIfShow(this);
    } else {
        al::hideModelIfShow(mEmptyActor);
    }

    mBaseQuat.set(al::getQuat(this));
    al::tryAddDisplayOffset(this, rInfo);
    mAppearTrans.set(al::getTrans(this));
    mConnector = al::tryCreateMtxConnector(this, rInfo);

    mAssistRotate = new ItemStateAssistRotate(this, &sShardsAssistRotateParam);

    if (mConnector != nullptr) {
        mAssistRotate->setRotateDegreePtr(&mRotateY);
    }

    al::initNerveState(this, mAssistRotate, &NrvShardsSpinDrc, "DRC回転");

    mPopUpFront = new ItemStatePopUpFront(this);
    al::initNerveState(this, mPopUpFront, &NrvShardsPopUpFront, "[state]跳ね上げ(前方)");

    if (mCollected) {
        mPopUpFront->changeModel(mEmptyActor);
    }

    al::tryGetArg(&mIsInvalidClipping, rInfo, "IsInvalidClipping");

    if (mIsInvalidClipping) {
        invalidateClippingAll();
    }

    al::tryGetArg(&mIsNotUseLpp, rInfo, "IsNotUseLpp");
    al::tryGetArg(&mIsConnectOnlyTrans, rInfo, "IsConnectOnlyTrans");
    al::tryGetArg(&mIsPlacementInRouteDokan, rInfo, "IsPlacementInRouteDokan");
    al::tryGetArg(&mIsDropShadowActorDown, rInfo, "IsDropShadowActorDown");
    al::tryGetArg(&mIsForceWaitAppear, rInfo, "IsForceWaitAppear");
    mIsSwitchAppear = al::listenStageSwitchOnOffAppear(
        this, al::FunctorV0M(static_cast<al::LiveActor*>(this), &al::LiveActor::appear),
        al::FunctorV0M(this, &Shards::switchKill));

    f32 shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, rInfo, "ShadowLength");

    if (shadowLength > 0.0f) {
        al::setShadowDropLength(this, shadowLength, "GreenStar");
    }

    bool isExpandClipping = false;
    al::tryGetArg(&isExpandClipping, rInfo, "IsExpandClippingShadowLength");

    if (isExpandClipping) {
        al::tryExpandClippingByShadowLength(this, &mClippingShadowExpand);
        al::tryExpandClippingByShadowLength(mEmptyActor, &mClippingShadowExpand);
    }

    if (mIsSwitchAppear) {
        bool isUseFocusCamera = false;
        al::tryGetArg(&isUseFocusCamera, rInfo, "IsUseFocusCamera");

        if (isUseFocusCamera) {
            s32 interpoleStep = 30;
            al::tryGetArg(&interpoleStep, rInfo, "DemoCameraInterpoleStep");
            mDemoCameraParam = new ActorStateDemoCameraParam(interpoleStep + 120, 30, interpoleStep,
                                                             nullptr, nullptr);
            mDemoCameraName = new sead::FixedSafeString<32>();
            mDemoCameraName->format("AppearGreenStar%d", mShardId);
            mDemoCamera = new ActorStateDemoCamera(this, rInfo, mDemoCameraName->cstr(),
                                                   mDemoCameraParam, false);
            al::initNerveState(this, mDemoCamera, &NrvShardsDemoAppear, "出現デモ");
        }

        bool isCountAsAlive = false;
        al::tryGetArg(&isCountAsAlive, rInfo, "IsCountAsAlive");

        if (isCountAsAlive) {
            getFlags()->isDeadAlive = true;
        }
    }

    if (mIsAttach) {
        initCollider(80.0f, 0.0f, 0);
        al::offCollide(this);
        al::setNerve(this, &NrvShardsAttached);
    }

    mIsInitialized = true;
}

/**
 * @brief Stage switch callback: kill the shard, with its disappear animation when visible.
 */
void Shards::switchKill() {
    if (al::isClipped(this)) {
        kill();
    } else {
        al::setNerve(this, &NrvShardsDirectDisappear);
    }
}

/**
 * @brief Appear or stay dead depending on the placement, then connect to the ground.
 */
void Shards::initAfterPlacement() {
    if (mIsSwitchAppear) {
        makeActorDead();
    } else if (!mIsKeepDeadAfterPlacement) {
        makeActorAppeared();
    }

    if (mConnector != nullptr) {
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 400.0f);

        if (al::isMtxConnectorConnecting(mConnector)) {
            mAssistRotate->setConnector(mConnector, mBaseQuat);
        }
    }

    al::updateMaterialCodeWater(this);
    al::calcFrontDir(&mGoalFront, this);
}

/**
 * @brief Respawn the shard, unless it is managed by a ShardsWatcher.
 */
void Shards::respawn() {
    if (!mFromWatcher) {
        respawnShard();
    }
}

/**
 * @brief Respawn the shard: revive an already collected one, or reset the clipping of an attached
 * one.
 */
void Shards::respawnShard() {
    if (al::isDead(this)) {
        if (mEmptyActor != nullptr && mCollected) {
            mCollectionFinished = false;
            makeActorAppeared();
            mCollectionFinished = true;
        }
    } else if (mIsAttach && !mIsInvalidClipping) {
        al::invalidateClipping(this);
        al::validateClipping(this);

        if (mEmptyActor != nullptr) {
            al::invalidateClipping(mEmptyActor);
            al::validateClipping(mEmptyActor);
        }
    }
}

/**
 * @brief Make the shard appear and start its appear behavior.
 */
void Shards::makeActorAppeared() {
    if (mCollectionFinished) {
        return;
    }

    al::LiveActor::makeActorAppeared();

    if (mEmptyActor != nullptr && mCollected) {
        al::hideModelIfShow(this);
        mEmptyActor->makeActorAppeared();
        al::showModelIfHide(mEmptyActor);
        mPopUpFront->changeModel(mEmptyActor);
    } else {
        mPopUpFront->changeModel(nullptr);
    }

    if (!al::isValidSwitchAppear(this)) {
        al::setNerve(this, &NrvShardsWait);
    } else if (mDemoCamera != nullptr && mDemoCamera->tryStart(&NrvShardsDemoAppear)) {
        al::hideModelIfShow(this);
    } else if (mIsForceWaitAppear) {
        startAction("DirectAppear");
        calcAnim();
        al::setNerve(this, &NrvShardsDirectAppear);
    } else {
        al::setNerve(this, &NrvShardsAppear);
    }

    al::tryOnStageSwitch(this, "ObjSyncSwitchKeepOn");
}

/**
 * @brief Start an action on the visible model.
 * @param pActionName The action name.
 */
void Shards::startAction(const char* pActionName) {
    if (mEmptyActor != nullptr && mCollected) {
        al::startAction(mEmptyActor, pActionName);
    } else {
        al::startAction(this, pActionName);
    }
}

/**
 * @brief Kill the shard without the kill sequence.
 */
void Shards::makeActorDead() {
    al::LiveActor::makeActorDead();
    al::tryOffStageSwitch(this, "ObjSyncSwitchKeepOn");
}

/**
 * @brief Start clipping.
 */
void Shards::startClipped() {
    al::LiveActor::startClipped();
}

/**
 * @brief End clipping.
 */
void Shards::endClipped() {
    al::LiveActor::endClipped();
}

/**
 * @brief Spin when blown into the microphone and keep the collected model on the shard.
 */
void Shards::control() {
    if (al::isMicBreathInputOn(this) &&
        (al::isNerve(this, &NrvShardsWait) || al::isNerve(this, &NrvShardsAttached) ||
         al::isNerve(this, &NrvShardsSpinDrc))) {
        al::setNerve(this, &NrvShardsSpinDrc);
        return;
    }

    if (mEmptyActor != nullptr && mCollected) {
        al::copyPose(mEmptyActor, this);
    }
}

/**
 * @brief Appear at a position with the appear arc.
 * @param rPos The appear position.
 */
void Shards::appearWithPos(const sead::Vector3f& rPos) {
    invalidateClippingAll();
    al::setTrans(this, rPos);
    mAppearTrans.set(rPos);
    appear();
    al::invalidateHitSensors(this);
    al::setNerve(this, &NrvShardsAppear);
}

/**
 * @brief Stage switch callback: appear with the appear arc.
 */
void Shards::appearBySwitch() {
    if (mCollectionFinished) {
        return;
    }

    startAction("Appear");
    appear();
    al::invalidateHitSensors(this);
    al::setNerve(this, &NrvShardsAppear);
}

/**
 * @brief Check whether a message collects the shard.
 * @param pMsg The received message.
 * @param pOther The sensor that sent the message.
 * @return Whether the message collects the shard.
 */
bool Shards::isEnableMsgItemGet(const al::SensorMsg* pMsg, al::HitSensor* pOther) const {
    if (mIsPlacementInRouteDokan) {
        return rc::isMsgRouteDokanItemGet(pMsg);
    }

    return (al::isMsgBallItemGet(pMsg) && al::isSensorHostName(pOther, "NekoNormal")) ||
           al::isMsgItemGetAll(pMsg) || rc::isMsgPackunEat(pMsg);
}

/**
 * @brief Handle a sensor message.
 * @param pMsg The received message.
 * @param pOther The sensor that sent the message.
 * @param pSelf The sensor of this shard that received it.
 * @return Whether the message was handled.
 */
bool Shards::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isClipped(this)) {
        return false;
    }

    if (mIsAttach) {
        if (al::isNerve(this, &NrvShardsDisappear) || al::isNerve(this, &NrvShardsGot)) {
            return false;
        }

        if (rc::isMsgItemBubbleBreak(pMsg)) {
            if (!mCollecting) {
                if (rc::isInWaterArea(this)) {
                    al::setNerve(this, &NrvShardsWait);
                } else {
                    mPopUpFront->setParamDefaultAbove();
                    mPopUpFront->setParamOnCollide();
                    al::onCollide(this);
                    al::setNerve(this, &NrvShardsPopUpFront);
                }
            }

            mIsAttach = false;
            return true;
        }
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        if (al::isNerve(this, &NrvShardsDirectDisappear)) {
            return false;
        }

        if (al::isNerve(this, &NrvShardsDisappear)) {
            return false;
        }

        return !al::isNerve(this, &NrvShardsGot);
    }

    if (al::isNerve(this, &NrvShardsWait) || al::isNerve(this, &NrvShardsSpinDrc) ||
        al::isNerve(this, &NrvShardsAttached)) {
        if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
            al::isMsgEnemyAttack(pMsg)) {
            if (al::isNerve(this, &NrvShardsSpinDrc)) {
                mIsSpinByAttack = true;
            }

            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvShardsSpinDrc);
            return true;
        }

        bool isKoopaFire = al::isSensorHostName(pOther, "KoopaSuperFireBallBig") ||
                           al::isSensorHostName(pOther, "KoopaFireBall");
        bool isExplosion = al::isMsgExplosion(pMsg);

        if (isKoopaFire || isExplosion || al::isMsgNpcTouch(pMsg)) {
            if (al::isNerve(this, &NrvShardsSpinDrc)) {
                mIsSpinByAttack = true;
            }

            al::setNerve(this, &NrvShardsSpinDrc);

            if (isKoopaFire) {
                return al::isMsgEnemyAttackFire(pMsg) && al::isSensorName(pOther, "Attack");
            }

            if (!al::isMsgExplosion(pMsg)) {
                return true;
            }
        }
    }

    if (!isEnableMsgItemGet(pMsg, pOther)) {
        return false;
    }

    if (al::isNerve(this, &NrvShardsAppear) && al::isLessStep(this, 20)) {
        return false;
    }

    if (al::isNerve(this, &NrvShardsDirectAppear) || al::isNerve(this, &NrvShardsDemoAppear) ||
        al::isNerve(this, &NrvShardsDirectDisappear) || al::isNerve(this, &NrvShardsDisappear) ||
        al::isNerve(this, &NrvShardsGot) || al::isNerve(this, &NrvShardsGotLast)) {
        return false;
    }

    doGet(pOther, pSelf);
    return true;
}

/**
 * @brief Collect the shard.
 * @param pOther The sensor that collected the shard.
 * @param pSelf The sensor of this shard.
 */
void Shards::doGet(al::HitSensor* pOther, al::HitSensor* pSelf) {
    auto* goalItemHolder = al::tryGetSceneObj<GoalItemHolder>(this, SceneObjID_GoalItemHolder);

    if (goalItemHolder != nullptr && goalItemHolder->getCurrentGoalItem() != nullptr) {
        return;
    }

    if (mCollected) {
        al::setAppearItemAttackerSensor(mEmptyActor, pOther);
        al::appearItemTiming(mEmptyActor, "撫でる");

        if (mEmptyActor != nullptr) {
            al::startSe(mEmptyActor, "PgGetEmpty", nullptr);
        } else {
            al::startSe(this, "PgGetEmpty", nullptr);
        }

        al::startHitReactionGet(mEmptyActor);
    } else if (mWatcher->isFinalShard()) {
        al::startSe(this, "PgGetAll", nullptr);
    } else {
        al::startSe(this, "PgGetAdd", nullptr);
    }

    rc::addScore(this, pOther, 0.0f, 0);
    mCollectSensor = pOther;
    al::startHitReactionGet(this);
    mCollectionFinished = true;

    if (al::isSensorKoopaJr(pOther)) {
        mCollectedBySensor = true;
    }

    getFlags()->isDeadAlive = false;

    if (mCollected) {
        al::setNerve(this, &NrvShardsDisappear);
        return;
    }

    if (mWatcher->isFinalShard()) {
        ShardsWatcher::sIsFinalShardGetPending = true;
    }

    invalidateClippingAll();
    al::setNerve(this, &NrvShardsGot);
}

/**
 * @brief Check whether the action of the visible model ended.
 * @return Whether the action ended.
 */
bool Shards::isActionEnd() {
    if (mEmptyActor != nullptr && mCollected) {
        return al::isActionEnd(mEmptyActor);
    }

    return al::isActionEnd(this);
}

/**
 * @brief Check whether the shard was collected.
 * @return Whether the shard is in a collected state.
 */
bool Shards::isGot() const {
    return al::isNerve(this, &NrvShardsGot) || al::isNerve(this, &NrvShardsGotLast) ||
           al::isNerve(this, &NrvShardsDisappear);
}

/**
 * @brief Handle a touch screen message: spin when tapped.
 * @param pMsg The received message.
 * @param pPointer The screen pointer.
 * @param pTarget The screen point target.
 * @return Whether the message was handled.
 */
bool Shards::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                   al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvShardsDirectDisappear) || al::isNerve(this, &NrvShardsDisappear) ||
        al::isNerve(this, &NrvShardsGot) || al::isNerve(this, &NrvShardsDemoAppear) ||
        al::isNerve(this, &NrvShardsPopUpFront) || al::isNerve(this, &NrvShardsAttached)) {
        return false;
    }

    if (al::isMsgTouchAssistNoPat(pMsg)) {
        al::setNerve(this, &NrvShardsSpinDrc);
        return true;
    }

    return false;
}

/**
 * @brief Follow the island the shard is placed on.
 * @param rTrans The new position.
 */
void Shards::updateLinkedTrans(const sead::Vector3f& rTrans) {
    if (!mIgnoreIslandOffset) {
        al::setTrans(this, rTrans);
    }
}

/**
 * @brief Remember whether the shard was clipped when a demo started.
 * @param demoType The demo type.
 */
void Shards::startDemoActor(s32 demoType) {
    if (al::isAlive(this) && (!al::isClipped(this) || !al::isInvalidClipping(this))) {
        mIsClippedInDemo = true;
    }
}

/**
 * @brief Reset the demo clipping state.
 * @param demoType The demo type.
 */
void Shards::endDemoActor(s32 demoType) {
    mIsClippedInDemo = false;

    // The result of the clipping check is unused.
    if (al::isAlive(this)) {
        al::isClipped(this);
    }
}

/**
 * @brief Check whether this is the last shard of its ShardsWatcher.
 * @return Whether this is the final shard.
 */
bool Shards::isFinalShard() const {
    return mWatcher != nullptr && mWatcher->isFinalShard();
}

/**
 * @brief Disconnect the shard from the collision it is placed on.
 */
void Shards::setNoConnect() {
    if (mConnector != nullptr) {
        mConnector->clear();
    }
}

/**
 * @brief Read the shard ID from a placement.
 * @param rInfo The placement info.
 */
void Shards::setShardId(const al::PlacementInfo& rInfo) {
    al::tryGetArg(&mShardId, rInfo, "ShardID");
}

/**
 * @brief Spin the shard around its up axis.
 * @param speed The rotation speed in degrees per frame.
 */
void Shards::rotate(f32 speed) {
    if (mConnector != nullptr && al::isMtxConnectorConnecting(mConnector)) {
        mRotateY = al::wrapAngle(mRotateY + speed);
        sead::Quatf quat;
        al::rotateQuatYDirDegree(&quat, mBaseQuat, mRotateY);

        if (mIsConnectOnlyTrans) {
            al::connectPoseTrans(this, mConnector, al::getConnectBaseTrans(mConnector));
            al::rotateQuatYDirDegree(this, al::getQuat(this), speed);
        } else {
            al::connectPoseQT(this, mConnector, quat, al::getConnectBaseTrans(mConnector));
        }
    } else {
        al::rotateQuatYDirDegree(this, al::getQuat(this), speed);
    }
}

/**
 * @brief Appear nerve: jump up out of the ground.
 */
void Shards::exeAppear() {
    if (al::isFirstStep(this)) {
        startAction("Appear");
    }

    al::updateMaterialCodeWater(this);
    f32 rate = al::calcNerveRate(this, 120);
    updateAppearMove(al::easeOut(rate));

    if (rate >= 1.0f) {
        al::validateHitSensors(this);
        al::setNerve(this, &NrvShardsWait);
    }
}

/**
 * @brief Direct appear nerve: appear in place.
 */
void Shards::exeDirectAppear() {
    if (al::isFirstStep(this)) {
        invalidateClippingAll();
    }

    rotate(3.0f);
    al::calcNerveRate(this, al::getActionFrameMax(this, "DirectAppear"));

    if (isActionEnd()) {
        al::setNerve(this, &NrvShardsWait);
    }
}

/**
 * @brief Wait nerve: spin in place.
 */
void Shards::exeWait() {
    if (al::isFirstStep(this)) {
        startAction("Wait");

        if (!mIsInvalidClipping) {
            validateClippingAll();
        }

        al::updateMaterialCodeWater(this);
    }

    updateDemoEffect();
    rotate(3.0f);
}

/**
 * @brief Attached nerve: spin while attached to the host.
 */
void Shards::exeAttached() {
    if (al::isFirstStep(this)) {
        startAction("Wait");
    }

    updateDemoEffect();
    rotate(3.0f);
}

/**
 * @brief Throw nerve: fall until hitting the ground.
 */
void Shards::exeThrow() {
    if (al::isFirstStep(this)) {
        startAction("Throw");
    }

    al::scaleVelocity(this, 0.98f);
    al::addVelocity(this, sead::Vector3f::ey * -1.5f);
    rotate(3.0f);

    if (alCollisionUtil::checkStrikeArrow(this, al::getTrans(this), sead::Vector3f::ey * -150.0f,
                                          nullptr, nullptr)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvShardsWait);
    }
}

/**
 * @brief Spin nerve: spin fast after being hit or touched.
 */
void Shards::exeSpinDrc() {
    if (al::isFirstStep(this)) {
        if (mEmptyActor != nullptr && mCollected) {
            al::startAction(mEmptyActor, "SpinDrc");

            if (mIsSpinByAttack) {
                mIsSpinByAttack = false;
            } else {
                al::startHitReaction(mEmptyActor, "Spin");
            }
        } else {
            al::tryStartActionIfNotPlaying(this, "SpinDrc");

            if (mIsSpinByAttack) {
                mIsSpinByAttack = false;
            } else {
                al::startHitReaction(this, "Spin");
            }
        }

        mAssistRotate->setRotateDegree(mRotateY);
    }

    if (!mIsSpinSeStarted) {
        if (mEmptyActor != nullptr && mCollected) {
            al::startSe(mEmptyActor, "PgSpin", nullptr);
        } else {
            al::startSe(this, "PgSpin", nullptr);
        }

        mIsSpinSeStarted = true;
    }

    if (al::isStep(this, 2)) {
        mIsSpinSeStarted = false;
    }

    if (al::updateNerveState(this)) {
        al::LiveActor* effectActor = mCollected ? mEmptyActor : this;
        al::tryDeleteEffect(effectActor, "Spin");

        if (mIsAttach) {
            al::setNerve(this, &NrvShardsAttached);
        } else {
            al::setNerve(this, &NrvShardsWait);
        }
    }
}

/**
 * @brief Demo appear nerve: appear with the focus camera.
 */
void Shards::exeDemoAppear() {
    if (al::isFirstStep(this)) {
        invalidateClippingAll();
        al::updateMaterialCodeWater(this);
    }

    s32 appearStep = mDemoCameraParam->_8 + 30;

    if (mDemoCamera->getPlayStep() == appearStep) {
        al::showModelIfHide(this);
        startAction("Appear");
    }

    if (appearStep < mDemoCamera->getPlayStep()) {
        f32 rate = al::normalize(static_cast<f32>(mDemoCamera->getPlayStep()),
                                 static_cast<f32>(appearStep),
                                 static_cast<f32>(mDemoCameraParam->_4));
        updateAppearMove(al::easeOut(rate));
    }

    if (al::updateNerveState(this)) {
        if (!mIsInvalidClipping) {
            validateClippingAll();
        }

        al::setNerve(this, &NrvShardsWait);
    }
}

/**
 * @brief Direct disappear nerve: disappear in place, then die.
 */
void Shards::exeDirectDisappear() {
    if (al::isFirstStep(this)) {
        invalidateClippingAll();
        startAction("DirectDisappear");
    }

    al::calcNerveRate(this, al::getActionFrameMax(this, "DirectDisappear"));

    if (isActionEnd()) {
        kill();
    }
}

/**
 * @brief Disappear nerve: an already collected shard was collected again.
 */
void Shards::exeDisappear() {
    if (al::isFirstStep(this)) {
        invalidateClippingAll();
        startAction("Got");
    }

    if (isActionEnd()) {
        al::tryOnSwitchDeadOn(this);
        kill();

        if (mEmptyActor != nullptr) {
            mEmptyActor->kill();
        }
    }
}

/**
 * @brief Got nerve: save the shard and update the shard counter.
 */
void Shards::exeGot() {
    if (al::isFirstStep(this)) {
        invalidateClippingAll();

        if (mDisconnectWhenGot && mConnector != nullptr &&
            al::isMtxConnectorConnecting(mConnector)) {
            mConnector->clear();
            al::setQuat(this, sead::Quatf::unit);
        }

        bool isNotFinal;

        if (mWatcher->isFinalShard()) {
            mWatcher->setShardId(mShardId);
            isNotFinal = false;
        } else {
            SingleModeDataFunction::collectShard(GameDataHolderWriter(GameDataHolderAccessor(this)),
                                                 mIslandId - 1, mShardId - 1);
            isNotFinal = true;
        }

        auto* layout =
            al::tryGetSceneObj<SingleModeSceneLayout>(this, SceneObjID_SingleModeSceneLayout);

        if (!isNotFinal) {
            if (layout != nullptr) {
                layout->startShardDemo();
                layout->addShard(mIslandId - 1, mShardId, true, mCollectedBySensor);
            }

            al::setNerve(this, &NrvShardsGotLast);
            mCollecting = true;
            return;
        }

        al::startAction(this, "Got");

        if (layout != nullptr) {
            layout->addShard(mIslandId - 1, mShardId, false, mCollectedBySensor);
        }
    }

    if (isActionEnd()) {
        al::tryOnSwitchDeadOn(this);
        mCollected = true;
        kill();
    }
}

/**
 * @brief Got last nerve: play the demo for the final shard.
 */
void Shards::exeGotLast() {
    if (al::isFirstStep(this)) {
        if (!rc::requestStartDemoInGameCutscene(this)) {
            al::setNerve(this, &NrvShardsGotLast);
            return;
        }

        rc::setDemoFullEffectUpdate(this, true);
        rc::cancelAllPlayersForDemo(this);
        rc::setDemoAudioType(this, static_cast<alSeFunction::DemoType>(3));
        rc::addDemoActor(this);
        startAction("Got");
        ShardsWatcher::sIsFinalShardGetPending = false;
    }

    if (al::isStep(this, 100)) {
        rc::addDemoActor(mWatcher);
        mGoalPending = true;
    }

    if (isActionEnd() && al::isGreaterEqualStep(this, 105)) {
        rc::requestEndDemoInGameCutscene(this);
        rc::requestStartDemoInGameCutscene(this);
        rc::addDemoActor(mWatcher);
        mCollecting = false;
        al::tryOnSwitchDeadOn(this);
        mCollected = true;
        kill();
    }
}

/**
 * @brief Pop up nerve: jump out of a broken bubble.
 */
void Shards::exePopUpFront() {
    if (al::updateNerveState(this)) {
        al::offCollide(this);
        al::setVelocityZero(this);
        al::setNerve(this, &NrvShardsWait);
    }
}
