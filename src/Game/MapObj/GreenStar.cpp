#include "MapObj/GreenStar.hpp"

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Light/LightIntensityDirector.hpp"
#include "Library/Light/LightIntensityFunction.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
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
#include "Library/PostProcessing/OccludedEffectDirector.hpp"
#include "Library/PostProcessing/OccludedEffectRequestInfo.hpp"
#include "Library/PostProcessing/OfxFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/ActorStateDemoCamera.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/ItemAssistRotateParam.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/MysteryHouseChecker.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(GreenStar, Appear);
    NERVE_DECL(GreenStar, SpinDrc);
    NERVE_DECL(GreenStar, PopUpFront);
    NERVE_DECL(GreenStar, DemoAppear);
    NERVE_DECL(GreenStar, Attached);
    NERVE_DECL(GreenStar, DirectAppear);
    NERVE_DECL(GreenStar, Wait);
    NERVE_DECL(GreenStar, Throw);
    NERVE_DECL(GreenStar, Disappear);
    NERVE_DECL(GreenStar, Got);
    NERVE_DECL(GreenStar, DirectDisappear);
    NERVES_MAKE_NOSTRUCT(GreenStar, Appear, SpinDrc, PopUpFront, DemoAppear, Attached, DirectAppear,
                         Wait, Throw, Disappear, Got, DirectDisappear)

    const ItemAssistRotateParam sGreenStarAssistRotateParam(60, 12.0f, true, 3.0f, 120);
}  // namespace

/**
 * @brief Construct a green star.
 * @param pName The actor name.
 * @param pBubble The item bubble holding the star, or nullptr.
 * @param isAttach Whether the star starts attached to its host (a bubble or an item spawner).
 */
GreenStar::GreenStar(const char* pName, ItemBubble* pBubble, bool isAttach)
    : al::LiveActor(pName), mBubble(pBubble), mIsAttach(isAttach) {}

/**
 * @brief Move the star up and down along its appear arc and spin it.
 * @param rate The eased appear progress, from 0 to 1.
 */
inline void GreenStar::updateAppearMove(f32 rate) {
    f32 height = sead::Mathf::sin(sead::Mathf::deg2rad(rate * 180.0f)) * 200.0f;
    sead::Vector3f trans(0.0f, height, 0.0f);
    trans += mAppearTrans;
    al::setTrans(this, trans);
    rotate(al::lerpValue(rate, 20.0f, 3.0f));
}

/**
 * @brief Initialize the green star from its placement.
 * @param rInfo The actor init info.
 */
void GreenStar::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::tryGetArg(&mGreenStarId, rInfo, "GreenStarId");
    al::tryGetArg(&mUsingOccludedEffect, rInfo, "UsingOccludedEffect");

    if (mUsingOccludedEffect) {
        mOccludedEffect =
            OfxFunction::getOccludedEffectDirector(this)->createInfoByPresetName("ObjSmall");
    }

    al::tryGetArg(&mDisconnectWhenGot, rInfo, "DisconnectWhenGot");
    rc::declareGreenStarId(this, mGreenStarId);
    bool isAcquired =
        GameDataFunction::isAcquireGreenStar(GameDataHolderAccessor(this), mGreenStarId);
    const char* suffix = mIsAttach || mBubble != nullptr ? "Attach" : nullptr;
    al::initNerve(this, &NrvGreenStarAppear, 3);
    al::initActorWithArchiveName(this, rInfo, isAcquired ? "GreenStarEmpty" : "GreenStar",
                                 suffix);
    mIsAcquired = isAcquired;
    mBaseQuat.set(al::getQuat(this));
    al::tryGetArg(&mSeType, rInfo, "SeType");
    al::tryAddDisplayOffset(this, rInfo);
    mAppearTrans.set(al::getTrans(this));
    mConnector = al::tryCreateMtxConnector(this, rInfo);

    mAssistRotate = new ItemStateAssistRotate(this, &sGreenStarAssistRotateParam);

    if (mConnector != nullptr) {
        mAssistRotate->setRotateDegreePtr(&mRotateY);
    }

    al::initNerveState(this, mAssistRotate, &NrvGreenStarSpinDrc, "DRC回転");

    mPopUpFront = new ItemStatePopUpFront(this);
    al::initNerveState(this, mPopUpFront, &NrvGreenStarPopUpFront, "[state]跳ね上げ(前方)");

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
    MysteryHouseCheckerFunction::tryRegisterGreenStar(this);
    al::tryGetArg(&mIsDropShadowActorDown, rInfo, "IsDropShadowActorDown");
    al::tryGetArg(&mIsForceWaitAppear, rInfo, "IsForceWaitAppear");
    bool isSwitchAppear = al::listenStageSwitchOnOffAppear(
        this, al::FunctorV0M(static_cast<al::LiveActor*>(this), &al::LiveActor::appear),
        al::FunctorV0M(this, &GreenStar::switchKill));

    f32 shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, rInfo, "ShadowLength");

    if (shadowLength > 0.0f) {
        al::setShadowDropLength(this, shadowLength, "GreenStar");
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
            mDemoCameraName->format("AppearGreenStar%d", mGreenStarId);
            mDemoCamera = new ActorStateDemoCamera(this, rInfo, mDemoCameraName->cstr(),
                                                   mDemoCameraParam, false);
            al::initNerveState(this, mDemoCamera, &NrvGreenStarDemoAppear, "出現デモ");
        }

        makeActorDead();
    } else {
        makeActorAppeared();
    }

    if (mIsAttach) {
        initCollider(80.0f, 0.0f, 0);
        al::offCollide(this);
        al::setNerve(this, &NrvGreenStarAttached);
    }
}

/**
 * @brief Connect the star to the collision it is placed on.
 */
void GreenStar::initAfterPlacement() {
    if (mConnector != nullptr) {
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 400.0f);

        if (al::isMtxConnectorConnecting(mConnector)) {
            mAssistRotate->setConnector(mConnector, mBaseQuat);
        }
    }

    al::updateMaterialCodeWater(this);
}

/**
 * @brief Make the star appear and start its appear behavior.
 */
void GreenStar::makeActorAppeared() {
    if (mIsAcquiredInScene) {
        return;
    }

    al::LiveActor::makeActorAppeared();

    if (al::isValidSwitchAppear(this)) {
        if (mDemoCamera != nullptr && mDemoCamera->tryStart(&NrvGreenStarDemoAppear)) {
            al::hideModelIfShow(this);
        } else if (mIsForceWaitAppear) {
            al::startAction(this, "DirectAppear");
            calcAnim();
            al::setNerve(this, &NrvGreenStarDirectAppear);
        } else {
            al::setNerve(this, &NrvGreenStarAppear);
        }
    } else if (mBubble != nullptr) {
        al::setNerve(this, &NrvGreenStarAttached);
    } else {
        al::setNerve(this, &NrvGreenStarWait);
    }

    al::tryOnStageSwitch(this, "ObjSyncSwitchKeepOn");
}

/**
 * @brief Kill the star without the kill sequence.
 */
void GreenStar::makeActorDead() {
    al::LiveActor::makeActorDead();
    al::tryOffStageSwitch(this, "ObjSyncSwitchKeepOn");
}

/**
 * @brief Update the model for the light exposure, the occluded effect and the microphone spin.
 */
void GreenStar::control() {
    f32 exposure = LightIntensityFunction::getLightIntensityDirector(this)->getExposure();

    if (exposure < 2.0f) {
        al::startVisAnimAndSetFrameAndStop(this, "GreenStar", 0.0f);
    } else {
        al::startVisAnimAndSetFrameAndStop(this, "GreenStar", 1.0f);
    }

    if (mIsDropShadowActorDown) {
        al::setShadowDropDirActorDown(this);
    }

    if (mUsingOccludedEffect &&
        (al::isNerve(this, &NrvGreenStarWait) || al::isNerve(this, &NrvGreenStarAttached) ||
         al::isNerve(this, &NrvGreenStarSpinDrc) || al::isNerve(this, &NrvGreenStarThrow))) {
        mOccludedEffect->requestByPos(al::getTrans(this));
    }

    if (al::isMicBreathInputOn(this) &&
        (al::isNerve(this, &NrvGreenStarWait) ||
         (al::isNerve(this, &NrvGreenStarAttached) && mBubble == nullptr) ||
         al::isNerve(this, &NrvGreenStarSpinDrc))) {
        al::setNerve(this, &NrvGreenStarSpinDrc);
    }
}

/**
 * @brief Appear at a position with the appear arc.
 * @param rPos The appear position.
 */
void GreenStar::appearWithPos(const sead::Vector3f& rPos) {
    al::invalidateClipping(this);
    al::setTrans(this, rPos);
    mAppearTrans.set(rPos);
    appear();
    al::setNerve(this, &NrvGreenStarAppear);
}

/**
 * @brief Check whether a message collects the star.
 * @param pMsg The received message.
 * @return Whether the message collects the star.
 */
bool GreenStar::isEnableMsgItemGet(const al::SensorMsg* pMsg) const {
    if (mIsPlacementInRouteDokan) {
        return rc::isMsgRouteDokanItemGet(pMsg);
    }

    return al::isMsgItemGetAll(pMsg) || rc::isMsgPackunEat(pMsg);
}

/**
 * @brief Handle a sensor message.
 * @param pMsg The received message.
 * @param pOther The sensor that sent the message.
 * @param pSelf The sensor of this star that received it.
 * @return Whether the message was handled.
 */
bool GreenStar::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                           al::HitSensor* pSelf) {
    if (mIsAttach) {
        if (al::isNerve(this, &NrvGreenStarDisappear) || al::isNerve(this, &NrvGreenStarGot)) {
            return false;
        }

        if (rc::isMsgItemBubbleBreak(pMsg)) {
            if (rc::isInWaterArea(this)) {
                al::setNerve(this, &NrvGreenStarWait);
            } else {
                if (mBubble != nullptr) {
                    mPopUpFront->setParamDefault();
                } else {
                    mPopUpFront->setParamDefaultAbove();
                }

                mPopUpFront->setParamOnCollide();
                al::onCollide(this);
                al::setNerve(this, &NrvGreenStarPopUpFront);
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
        return !al::isNerve(this, &NrvGreenStarDirectDisappear) &&
               !al::isNerve(this, &NrvGreenStarDisappear) &&
               !al::isNerve(this, &NrvGreenStarGot);
    }

    if (al::isNerve(this, &NrvGreenStarWait) || al::isNerve(this, &NrvGreenStarSpinDrc) ||
        (al::isNerve(this, &NrvGreenStarAttached) && mBubble == nullptr)) {
        if (al::isMsgPlayerFireBallAttack(pMsg)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvGreenStarSpinDrc);
            return true;
        }
    }

    if (!isEnableMsgItemGet(pMsg)) {
        return false;
    }

    if (al::isNerve(this, &NrvGreenStarAppear) && al::isLessStep(this, 20)) {
        return false;
    }

    if (al::isNerve(this, &NrvGreenStarDirectAppear) ||
        al::isNerve(this, &NrvGreenStarDemoAppear) ||
        al::isNerve(this, &NrvGreenStarDirectDisappear) ||
        al::isNerve(this, &NrvGreenStarDisappear) || al::isNerve(this, &NrvGreenStarGot)) {
        return false;
    }

    doGet(pOther, pSelf);
    return true;
}

/**
 * @brief Collect the star.
 * @param pOther The sensor that collected the star.
 * @param pSelf The sensor of this star.
 */
void GreenStar::doGet(al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (mIsAcquired) {
        al::startSe(this, "PgGetEmpty", nullptr);
    } else if (mSeType == 1) {
        al::startSe(this, "PgGetZelda", nullptr);
    } else {
        al::startSe(this, "PgGetAdd", nullptr);
    }

    rc::addScore(this, pOther, 0.0f, 0);
    mAcquirerSensor = pOther;
    al::startHitReactionGet(this);
    rc::sendMsgRequestPlayerGetReaction(pOther, pSelf, "グリーンスターゲット");
    rc::acquireGreenStarId(this, mGreenStarId);
    mIsAcquiredInScene = true;

    if (mIsAcquired) {
        al::setNerve(this, &NrvGreenStarDisappear);
        al::killPrePassLight(this, "グリーンスター体", 5);
    } else {
        al::setNerve(this, &NrvGreenStarGot);
    }
}

/**
 * @brief Handle a touch screen message: spin when tapped.
 * @param pMsg The received message.
 * @param pPointer The screen pointer.
 * @param pTarget The screen point target.
 * @return Whether the message was handled.
 */
bool GreenStar::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                      al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvGreenStarDirectDisappear) ||
        al::isNerve(this, &NrvGreenStarDisappear) || al::isNerve(this, &NrvGreenStarGot) ||
        al::isNerve(this, &NrvGreenStarDemoAppear) || al::isNerve(this, &NrvGreenStarPopUpFront)) {
        return false;
    }

    if (mBubble != nullptr && al::isNerve(this, &NrvGreenStarAttached)) {
        return false;
    }

    if (al::isMsgTouchAssistNoPat(pMsg)) {
        al::setNerve(this, &NrvGreenStarSpinDrc);
        return true;
    }

    return false;
}

/**
 * @brief Stage switch callback: disappear in place.
 */
void GreenStar::switchKill() {
    al::setNerve(this, &NrvGreenStarDirectDisappear);
}

/**
 * @brief Disconnect the star from the collision it is placed on.
 */
void GreenStar::setNoConnect() {
    if (mConnector != nullptr) {
        mConnector->clear();
    }
}

/**
 * @brief Spin the star around its up axis.
 * @param speed The rotation speed in degrees per frame.
 */
void GreenStar::rotate(f32 speed) {
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
void GreenStar::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }

    al::updateMaterialCodeWater(this);
    f32 rate = al::calcNerveRate(this, 120);
    updateAppearMove(al::easeOut(rate));

    if (rate >= 1.0f) {
        al::setNerve(this, &NrvGreenStarWait);
    }
}

/**
 * @brief Direct appear nerve: appear in place.
 */
void GreenStar::exeDirectAppear() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    rotate(3.0f);
    al::calcNerveRate(this, al::getActionFrameMax(this, "DirectAppear"));

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGreenStarWait);
    }
}

/**
 * @brief Wait nerve: spin in place.
 */
void GreenStar::exeWait() {
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
void GreenStar::exeAttached() {
    if (al::isFirstStep(this)) {
        if (mBubble != nullptr) {
            al::startAction(this, "WaitBubble");
        } else {
            al::startAction(this, "Wait");
        }
    }

    rotate(3.0f);

    if (mBubble != nullptr && al::isDead(mBubble)) {
        al::setNerve(this, &NrvGreenStarWait);
        mIsAttach = false;
    }
}

/**
 * @brief Throw nerve: fall until hitting the ground.
 */
void GreenStar::exeThrow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");
    }

    al::scaleVelocity(this, 0.98f);
    al::addVelocity(this, sead::Vector3f::ey * -1.5f);
    rotate(3.0f);

    if (alCollisionUtil::checkStrikeArrow(this, al::getTrans(this), sead::Vector3f::ey * -150.0f,
                                          nullptr, nullptr)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvGreenStarWait);
    }
}

/**
 * @brief Spin nerve: spin fast after being hit or touched.
 */
void GreenStar::exeSpinDrc() {
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
            al::setNerve(this, &NrvGreenStarAttached);
        } else {
            al::setNerve(this, &NrvGreenStarWait);
        }
    }
}

/**
 * @brief Demo appear nerve: appear with the focus camera.
 */
void GreenStar::exeDemoAppear() {
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

        al::setNerve(this, &NrvGreenStarWait);
    }
}

/**
 * @brief Direct disappear nerve: disappear in place, then die.
 */
void GreenStar::exeDirectDisappear() {
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
 * @brief Disappear nerve: an already collected star was collected again.
 */
void GreenStar::exeDisappear() {
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
void GreenStar::exeGot() {
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
void GreenStar::exePopUpFront() {
    if (al::updateNerveState(this)) {
        al::setVelocityZero(this);
        al::setNerve(this, &NrvGreenStarWait);
    }
}

/**
 * @brief Destroy the green star.
 */
GreenStar::~GreenStar() {}
