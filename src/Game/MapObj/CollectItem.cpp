#include "MapObj/CollectItem.hpp"

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Light/LightIntensityDirector.hpp"
#include "Library/Light/LightIntensityFunction.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/LiveActorFlag.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/ActorStateDemoCamera.hpp"
#include "MapObj/ItemAssistRotateParam.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(CollectItem, Appear);
    NERVE_DECL(CollectItem, SpinDrc);
    NERVE_DECL(CollectItem, PopUpFront);
    NERVE_DECL(CollectItem, DemoAppear);
    NERVE_DECL(CollectItem, Attached);
    NERVE_DECL(CollectItem, DirectAppear);
    NERVE_DECL(CollectItem, Wait);
    NERVE_DECL(CollectItem, Disappear);
    NERVE_DECL(CollectItem, Got);
    NERVE_DECL(CollectItem, DirectDisappear);
    NERVES_MAKE_NOSTRUCT(CollectItem, Appear, SpinDrc, PopUpFront, DemoAppear, Attached,
                         DirectAppear, Wait, Disappear, Got, DirectDisappear)

    const ItemAssistRotateParam sCollectItemAssistRotateParam(60, 12.0f, true, 3.0f, 120);
}  // namespace

/**
 * @brief Construct a collect item.
 * @param pName The actor name.
 * @param pBubble The item bubble holding the item, or nullptr.
 * @param isAttach Whether the item starts attached to its host (a bubble or an item spawner).
 */
CollectItem::CollectItem(const char* pName, ItemBubble* pBubble, bool isAttach)
    : al::LiveActor(pName), mBubble(pBubble), mIsAttach(isAttach) {}

/**
 * @brief Move the item up and down along its appear arc and spin it.
 * @param rate The eased appear progress, from 0 to 1.
 */
inline void CollectItem::updateAppearMove(f32 rate) {
    f32 height = sead::Mathf::sin(sead::Mathf::deg2rad(rate * 180.0f)) * 200.0f;
    sead::Vector3f trans(0.0f, height, 0.0f);
    trans += mAppearTrans;
    al::setTrans(this, trans);
    rotate(al::lerpValue(rate, 20.0f, 3.0f));
}

/**
 * @brief Initialize the collect item from its placement.
 * @param rInfo The actor init info.
 */
void CollectItem::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::tryGetArg(&mUsingOccludedEffect, rInfo, "UsingOccludedEffect");
    al::tryGetArg(&mDisconnectWhenGot, rInfo, "DisconnectWhenGot");
    const char* suffix = mIsAttach || mBubble != nullptr ? "Attach" : nullptr;
    al::initNerve(this, &NrvCollectItemAppear, 3);
    alPlacementFunction::tryGetModelName(&mModelName, rInfo);
    mModelName = mModelName != nullptr ? mModelName : "CollectItem";
    al::initActorWithArchiveName(this, rInfo, mModelName, suffix);
    mIsAcquired = false;
    mBaseQuat.set(al::getQuat(this));
    al::tryAddDisplayOffset(this, rInfo);
    mAppearTrans.set(al::getTrans(this));
    mConnector = al::tryCreateMtxConnector(this, rInfo);

    mAssistRotate = new ItemStateAssistRotate(this, &sCollectItemAssistRotateParam);

    if (mConnector != nullptr) {
        mAssistRotate->setRotateDegreePtr(&mRotateY);
    }

    al::initNerveState(this, mAssistRotate, &NrvCollectItemSpinDrc, "DRC回転");

    mPopUpFront = new ItemStatePopUpFront(this);
    al::initNerveState(this, mPopUpFront, &NrvCollectItemPopUpFront, "[state]跳ね上げ(前方)");

    al::tryGetArg(&mIsInvalidClipping, rInfo, "IsInvalidClipping");

    if (mIsInvalidClipping) {
        al::invalidateClipping(this);
    }

    al::tryGetArg(&mIsNotUseLpp, rInfo, "IsNotUseLpp");

    if (mIsNotUseLpp) {
        al::killPrePassLightAll(this, -1);
    }

    al::tryGetArg(&mIsConnectOnlyTrans, rInfo, "IsConnectOnlyTrans");
    al::tryGetArg(&mIsPlacementInRouteDokan, rInfo, "IsPlacementInRouteDokan");
    al::tryGetArg(&mIsDropShadowActorDown, rInfo, "IsDropShadowActorDown");
    al::tryGetArg(&mIsForceWaitAppear, rInfo, "IsForceWaitAppear");
    bool isSwitchAppear = al::listenStageSwitchOnOffAppear(
        this, al::FunctorV0M(static_cast<al::LiveActor*>(this), &al::LiveActor::appear),
        al::FunctorV0M(this, &CollectItem::switchKill));

    f32 shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, rInfo, "ShadowLength");

    if (shadowLength > 0.0f) {
        al::setShadowDropLength(this, shadowLength, mModelName);
    }

    bool isExpandClipping = false;
    al::tryGetArg(&isExpandClipping, rInfo, "IsExpandClippingShadowLength");

    if (isExpandClipping) {
        al::tryExpandClippingByShadowLength(this, &mClippingShadowExpand);
    }

    if (isSwitchAppear) {
        bool isUseFocusCamera = false;
        al::tryGetArg(&isUseFocusCamera, rInfo, "IsUseFocusCamera");

        if (isUseFocusCamera) {
            s32 interpoleStep = 30;
            al::tryGetArg(&interpoleStep, rInfo, "DemoCameraInterpoleStep");
            mDemoCameraParam = new ActorStateDemoCameraParam(interpoleStep + 120, 30, interpoleStep,
                                                             nullptr, nullptr);
            mDemoCameraName = new sead::FixedSafeString<32>();
            mDemoCameraName->format("AppearGreenStar%d", mDemoCameraId);
            mDemoCamera = new ActorStateDemoCamera(this, rInfo, mDemoCameraName->cstr(),
                                                   mDemoCameraParam, false);
            al::initNerveState(this, mDemoCamera, &NrvCollectItemDemoAppear, "出現デモ");
        }

        bool isCountAsAlive = false;
        al::tryGetArg(&isCountAsAlive, rInfo, "IsCountAsAlive");

        if (isCountAsAlive) {
            getFlags()->isDeadAlive = true;
        }

        makeActorDead();
    } else {
        makeActorAppeared();
    }

    if (mIsAttach) {
        initCollider(80.0f, 0.0f, 0);
        al::offCollide(this);
        al::setNerve(this, &NrvCollectItemAttached);
    }
}

/**
 * @brief Stage switch callback: disappear in place.
 */
void CollectItem::switchKill() {
    al::setNerve(this, &NrvCollectItemDirectDisappear);
}

/**
 * @brief Connect the item to the collision it is placed on.
 */
void CollectItem::initAfterPlacement() {
    if (mConnector != nullptr) {
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 400.0f);

        if (al::isMtxConnectorConnecting(mConnector)) {
            mAssistRotate->setConnector(mConnector, mBaseQuat);
        }
    }

    al::updateMaterialCodeWater(this);
}

/**
 * @brief Make the item appear and start its appear behavior.
 */
void CollectItem::makeActorAppeared() {
    if (mIsAcquiredInScene) {
        return;
    }

    al::LiveActor::makeActorAppeared();

    if (al::isValidSwitchAppear(this)) {
        if (mDemoCamera != nullptr && mDemoCamera->tryStart(&NrvCollectItemDemoAppear)) {
            al::hideModelIfShow(this);
        } else if (mIsForceWaitAppear) {
            al::startAction(this, "DirectAppear");
            calcAnim();
            al::setNerve(this, &NrvCollectItemDirectAppear);
        } else {
            al::setNerve(this, &NrvCollectItemAppear);
        }
    } else if (mBubble != nullptr) {
        al::setNerve(this, &NrvCollectItemAttached);
    } else {
        al::setNerve(this, &NrvCollectItemWait);
    }

    al::tryOnStageSwitch(this, "ObjSyncSwitchKeepOn");
}

/**
 * @brief Kill the item without the kill sequence.
 */
void CollectItem::makeActorDead() {
    al::LiveActor::makeActorDead();
    al::tryOffStageSwitch(this, "ObjSyncSwitchKeepOn");
}

/**
 * @brief Update the model for the light exposure and the microphone spin.
 */
void CollectItem::control() {
    f32 exposure = LightIntensityFunction::getLightIntensityDirector(this)->getExposure();

    if (exposure < 2.0f) {
        al::startVisAnimAndSetFrameAndStop(this, mModelName, 0.0f);
    } else {
        al::startVisAnimAndSetFrameAndStop(this, mModelName, 1.0f);
    }

    if (mIsDropShadowActorDown) {
        al::setShadowDropDirActorDown(this);
    }

    if (al::isMicBreathInputOn(this) &&
        (al::isNerve(this, &NrvCollectItemWait) ||
         (al::isNerve(this, &NrvCollectItemAttached) && mBubble == nullptr) ||
         al::isNerve(this, &NrvCollectItemSpinDrc))) {
        al::setNerve(this, &NrvCollectItemSpinDrc);
    }
}

/**
 * @brief Appear at a position with the appear arc.
 * @param rPos The appear position.
 */
void CollectItem::appearWithPos(const sead::Vector3f& rPos) {
    al::invalidateClipping(this);
    al::setTrans(this, rPos);
    mAppearTrans.set(rPos);
    appear();
    al::setNerve(this, &NrvCollectItemAppear);
}

/**
 * @brief Appear with the appear arc when the appear switch turns on.
 */
void CollectItem::appearBySwitch() {
    if (mIsAcquiredInScene) {
        return;
    }

    al::startAction(this, "Appear");
    appear();
    al::setNerve(this, &NrvCollectItemAppear);
}

/**
 * @brief Check whether a message collects the item.
 * @param pMsg The received message.
 * @return Whether the message collects the item.
 */
bool CollectItem::isEnableMsgItemGet(const al::SensorMsg* pMsg) const {
    if (mIsPlacementInRouteDokan) {
        return rc::isMsgRouteDokanItemGet(pMsg);
    }

    return al::isMsgItemGetAll(pMsg) || rc::isMsgPackunEat(pMsg);
}

/**
 * @brief Handle a sensor message.
 * @param pMsg The received message.
 * @param pOther The sensor that sent the message.
 * @param pSelf The sensor of this item that received it.
 * @return Whether the message was handled.
 */
bool CollectItem::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                             al::HitSensor* pSelf) {
    if (mIsAttach) {
        if (al::isNerve(this, &NrvCollectItemDisappear) || al::isNerve(this, &NrvCollectItemGot)) {
            return false;
        }

        if (rc::isMsgItemBubbleBreak(pMsg)) {
            if (rc::isInWaterArea(this)) {
                al::setNerve(this, &NrvCollectItemWait);
            } else {
                if (mBubble != nullptr) {
                    mPopUpFront->setParamDefault();
                } else {
                    mPopUpFront->setParamDefaultAbove();
                }

                mPopUpFront->setParamOnCollide();
                al::onCollide(this);
                al::setNerve(this, &NrvCollectItemPopUpFront);
            }

            mIsAttach = false;
            return true;
        }

        if (mBubble != nullptr) {
            if (!rc::isMsgItemBubbleBreakAndGetItem(pMsg)) {
                return false;
            }

            if (!mBubble->isEnableGetPlayerSensor()) {
                return false;
            }

            doGet(mBubble->getHitPlayerSensor(), pSelf);
            mIsAttach = false;
            return true;
        }
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        return !al::isNerve(this, &NrvCollectItemDirectDisappear) &&
               !al::isNerve(this, &NrvCollectItemDisappear) &&
               !al::isNerve(this, &NrvCollectItemGot);
    }

    if (al::isNerve(this, &NrvCollectItemWait) || al::isNerve(this, &NrvCollectItemSpinDrc) ||
        (al::isNerve(this, &NrvCollectItemAttached) && mBubble == nullptr)) {
        if (al::isMsgPlayerFireBallAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvCollectItemSpinDrc);
            return true;
        }
    }

    if (!isEnableMsgItemGet(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvCollectItemAppear) && al::isLessStep(this, 20)) {
        return false;
    }

    if (al::isNerve(this, &NrvCollectItemDirectAppear) ||
        al::isNerve(this, &NrvCollectItemDemoAppear) ||
        al::isNerve(this, &NrvCollectItemDirectDisappear) ||
        al::isNerve(this, &NrvCollectItemDisappear) || al::isNerve(this, &NrvCollectItemGot)) {
        return false;
    }

    doGet(pOther, pSelf);
    return true;
}

/**
 * @brief Collect the item.
 * @param pOther The sensor that collected the item.
 * @param pSelf The sensor of this item.
 */
void CollectItem::doGet(al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (mIsAcquired) {
        al::startSe(this, "PgGetEmpty", nullptr);
    } else {
        al::startSe(this, "PgGetAdd", nullptr);
    }

    rc::addScore(this, pOther, 0.0f, 0);
    mAcquirerSensor = pOther;
    al::startHitReactionGet(this);
    mIsAcquiredInScene = true;
    getFlags()->isDeadAlive = false;

    if (mIsAcquired) {
        al::setNerve(this, &NrvCollectItemDisappear);
        al::killPrePassLight(this, "グリーンスター体", 5);
    } else {
        al::setNerve(this, &NrvCollectItemGot);
    }
}

/**
 * @brief Handle a touch screen message: spin when tapped.
 * @param pMsg The received message.
 * @param pPointer The screen pointer.
 * @param pTarget The screen point target.
 * @return Whether the message was handled.
 */
bool CollectItem::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvCollectItemDirectDisappear) ||
        al::isNerve(this, &NrvCollectItemDisappear) || al::isNerve(this, &NrvCollectItemGot) ||
        al::isNerve(this, &NrvCollectItemDemoAppear) ||
        al::isNerve(this, &NrvCollectItemPopUpFront)) {
        return false;
    }

    if (mBubble != nullptr && al::isNerve(this, &NrvCollectItemAttached)) {
        return false;
    }

    if (al::isMsgTouchAssistNoPat(pMsg)) {
        al::setNerve(this, &NrvCollectItemSpinDrc);
        return true;
    }

    return false;
}

/**
 * @brief Disconnect the item from the collision it is placed on.
 */
void CollectItem::setNoConnect() {
    if (mConnector != nullptr) {
        mConnector->clear();
    }
}

/**
 * @brief Spin the item around its up axis.
 * @param speed The rotation speed in degrees per frame.
 */
void CollectItem::rotate(f32 speed) {
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
void CollectItem::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }

    al::updateMaterialCodeWater(this);
    f32 rate = al::calcNerveRate(this, 120);
    updateAppearMove(al::easeOut(rate));

    if (rate >= 1.0f) {
        al::setNerve(this, &NrvCollectItemWait);
    }
}

/**
 * @brief Direct appear nerve: appear in place.
 */
void CollectItem::exeDirectAppear() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    rotate(3.0f);
    al::calcNerveRate(this, al::getActionFrameMax(this, "DirectAppear"));

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCollectItemWait);
    }
}

/**
 * @brief Wait nerve: spin in place.
 */
void CollectItem::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");

        if (!mIsInvalidClipping) {
            al::validateClipping(this);
        }

        al::updateMaterialCodeWater(this);
    }

    rotate(3.0f);
}

/**
 * @brief Attached nerve: spin while attached to the host, until the bubble is gone.
 */
void CollectItem::exeAttached() {
    if (al::isFirstStep(this)) {
        if (mBubble != nullptr) {
            al::startAction(this, "WaitBubble");
        } else {
            al::startAction(this, "Wait");
        }
    }

    rotate(3.0f);

    if (mBubble != nullptr && al::isDead(mBubble)) {
        al::setNerve(this, &NrvCollectItemWait);
        mIsAttach = false;
    }
}

/**
 * @brief Throw nerve: fall until hitting the ground.
 */
void CollectItem::exeThrow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
    }

    al::scaleVelocity(this, 0.98f);
    al::addVelocity(this, sead::Vector3f::ey * -1.5f);
    rotate(3.0f);

    if (alCollisionUtil::checkStrikeArrow(this, al::getTrans(this), sead::Vector3f::ey * -150.0f,
                                          nullptr, nullptr)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvCollectItemWait);
    }
}

/**
 * @brief Spin nerve: spin fast after being hit or touched.
 */
void CollectItem::exeSpinDrc() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "SpinDrc");
        mAssistRotate->setRotateDegree(mRotateY);
    }

    if (!mIsSpinSeStarted) {
        al::startSe(this, "PgSpin", nullptr);
        mIsSpinSeStarted = true;
    }

    if (al::isStep(this, 2)) {
        mIsSpinSeStarted = false;
    }

    al::requestPrePassLightColor(this, "グリーンスター体", "タッチ時の色", 1.0f);

    if (al::updateNerveState(this)) {
        if (mIsAttach) {
            al::setNerve(this, &NrvCollectItemAttached);
        } else {
            al::setNerve(this, &NrvCollectItemWait);
        }
    }
}

/**
 * @brief Demo appear nerve: appear with the focus camera.
 */
void CollectItem::exeDemoAppear() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::updateMaterialCodeWater(this);
    }

    s32 appearStep = mDemoCameraParam->_8 + 30;

    if (mDemoCamera->getPlayStep() == appearStep) {
        al::showModelIfHide(this);
        al::startAction(this, "Appear");
    }

    if (appearStep < mDemoCamera->getPlayStep()) {
        f32 rate = al::normalize(static_cast<f32>(mDemoCamera->getPlayStep()),
                                 static_cast<f32>(appearStep),
                                 static_cast<f32>(mDemoCameraParam->_4));
        updateAppearMove(al::easeOut(rate));
    }

    if (al::updateNerveState(this)) {
        if (!mIsInvalidClipping) {
            al::validateClipping(this);
        }

        al::setNerve(this, &NrvCollectItemWait);
    }
}

/**
 * @brief Direct disappear nerve: disappear in place, then die.
 */
void CollectItem::exeDirectDisappear() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "DirectDisappear");
    }

    al::calcNerveRate(this, al::getActionFrameMax(this, "DirectDisappear"));

    if (al::isActionEnd(this)) {
        kill();
    }
}

/**
 * @brief Disappear nerve: an already collected item was collected again.
 */
void CollectItem::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "Disappear");
    }

    if (al::isActionEnd(this)) {
        al::tryOnSwitchDeadOn(this);
        kill();
    }
}

/**
 * @brief Got nerve: play the collect animation, then die.
 */
void CollectItem::exeGot() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "Got");

        if (mDisconnectWhenGot && mConnector != nullptr &&
            al::isMtxConnectorConnecting(mConnector)) {
            mConnector->clear();
            al::setQuat(this, sead::Quatf::unit);
        }
    }

    if (al::isActionEnd(this)) {
        al::killPrePassLight(this, "グリーンスター体", 5);
        al::tryOnSwitchDeadOn(this);
        kill();
    }
}

/**
 * @brief Pop up nerve: jump out of a broken bubble.
 */
void CollectItem::exePopUpFront() {
    if (al::updateNerveState(this)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvCollectItemWait);
    }
}

/**
 * @brief Destroy the collect item.
 */
CollectItem::~CollectItem() {}
