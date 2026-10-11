#include "MapObj/KinokoOneUp.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/ItemStatePopUpAbove.hpp"
#include "MapObj/KinokoStateRunaway.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(KinokoOneUp, Wait);
NERVE_DECL(KinokoOneUp, PopUpFront);
NERVE_DECL(KinokoOneUp, PopUpAbove);
NERVE_DECL(KinokoOneUp, Runaway);
NERVE_DECL(KinokoOneUp, AttachBubble);
NERVE_DECL(KinokoOneUp, AppearWait);
NERVE_DECL(KinokoOneUp, ForceGet);
NERVES_MAKE_STRUCT(KinokoOneUp, Wait, PopUpFront, PopUpAbove, Runaway, AttachBubble)
NERVES_MAKE_NOSTRUCT(KinokoOneUp, AppearWait, ForceGet)
ItemStatePopUpFrontParam sOneUpTakeOutParam;
}
KinokoOneUp::KinokoOneUp(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
KinokoOneUp::~KinokoOneUp() {}
void KinokoOneUp::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "KinokoOneUp", nullptr);
    al::initNerve(this, &NrvKinokoOneUp.Wait, 3);
    mPopUpFront = new ItemStatePopUpFront(this);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    al::initNerveState(this, mPopUpFront, &NrvKinokoOneUp.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mPopUpAbove, &NrvKinokoOneUp.PopUpAbove, "[state]跳ね上げ(真上)");
    auto* runaway = new KinokoStateRunaway(this);
    al::initNerveState(this, runaway, &NrvKinokoOneUp.Runaway, "[state]逃げる");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    if (mBubble) al::setNerve(this, &NrvKinokoOneUp.AttachBubble);
    al::tryGetArg(&mIsInRouteDokan, info, "IsPlacementInRouteDokan");
    al::updateEffectMaterialRouteDokan(this, mIsInRouteDokan);
    mConnector = al::tryCreateMtxConnector(this, info);
    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &KinokoOneUp::appearPopUpFront))) makeActorDead();
    else makeActorAppeared();
}
void KinokoOneUp::appearPopUpFront() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    if (mCollideOnPopUp) mPopUpFront->setParamOnCollide();
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    al::setNerve(this, &NrvKinokoOneUp.PopUpFront);
}
void KinokoOneUp::initAfterPlacement() {
    al::updateMaterialCodeArea(this);
    if (mConnector) { al::offCollide(this); al::attachMtxConnectorToCollision(mConnector, this, false); }
}
void KinokoOneUp::appear() {
    al::LiveActor::appear();
    if (mBubble) al::setNerve(this, &NrvKinokoOneUp.AttachBubble);
    else al::setNerve(this, &NrvKinokoOneUp.Wait);
}
void KinokoOneUp::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
    if (!mRumble->isEnd()) { mRumble->calc(); al::setScaleY(this, mRumble->getValueY() + 1.0f); }
    else al::setScaleY(this, 1.0f);
    al::updateMaterialCodeWater(this);
}
bool KinokoOneUp::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvKinokoOneUp.AttachBubble) || al::isNerve(this, &NrvKinokoOneUp.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvKinokoOneUp.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvKinokoOneUp.AttachBubble)) {
        if (!rc::isMsgItemBubbleBreakAndGetItem(msg)) return false;
        if (!mBubble->isEnableGetPlayerSensor()) return false;
        sender = mBubble->getHitPlayerSensor();
    } else {
        if (al::isMsgPlayerFireBallAttack(msg)) {
            if (mRumble->isEnd()) {
                al::startHitReactionHit(this);
                rc::requestHitReactionToAttacker(msg, receiver, sender);
                mRumble->start(0);
            }
            return true;
        }
        if (!isEnableMsgItemGet(msg)) return false;
    }
    rc::acquirerItemOneUp(this, sender);
    al::startHitReactionGet(this);
    rc::addScore(this, sender, 0.0f, 0);
    kill();
    return true;
}
bool KinokoOneUp::isEnableMsgItemGet(const al::SensorMsg* msg) const {
    if (mIsInRouteDokan) return rc::isMsgRouteDokanItemGet(msg);
    return al::isMsgItemGetAll(msg);
}
bool KinokoOneUp::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvKinokoOneUp.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void KinokoOneUp::appearWait() {
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    al::setNerve(this, &NrvKinokoOneUpAppearWait);
}
void KinokoOneUp::appearPopUpAbove() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    al::setNerve(this, &NrvKinokoOneUp.PopUpAbove);
}
void KinokoOneUp::appearItemTakeOut() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    sOneUpTakeOutParam.setDefault();
    sOneUpTakeOutParam._74 = false;
    mPopUpFront->setParam(sOneUpTakeOutParam, nullptr);
    al::LiveActor::appear();
    al::setNerve(this, &NrvKinokoOneUp.PopUpFront);
}
void KinokoOneUp::appearForceGet(const al::HitSensor* sensor) {
    mForceGetSensor = sensor;
    mForceGetHeight = 300.0f;
    mForceGetSpeed = 11.0f;
    al::setTrans(this, al::getActorTrans(sensor) + sead::Vector3f(0.0f, mForceGetHeight, 0.0f));
    al::LiveActor::appear();
    al::setNerve(this, &NrvKinokoOneUpForceGet);
}
void KinokoOneUp::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void KinokoOneUp::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void KinokoOneUp::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvKinokoOneUp.PopUpFront);
}
void KinokoOneUp::exeAppearWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear");
    if (al::isActionEnd(this)) { al::setNerve(this, &NrvKinokoOneUp.Wait); al::validateHitSensors(this); }
}
void KinokoOneUp::exePopUpFront() {
    al::updateNerveStateAndNextNerve(this, &NrvKinokoOneUp.Runaway);
    rc::startHitReactionIfThroughWater(this);
}
void KinokoOneUp::exePopUpAbove() {
    al::updateNerveStateAndNextNerve(this, &NrvKinokoOneUp.Runaway);
    rc::startHitReactionIfThroughWater(this);
}
void KinokoOneUp::exeRunaway() {
    if (al::isFirstStep(this)) al::startSe(this, "Land", nullptr);
    al::holdSe(this, "PgRunning", nullptr);
    rc::startHitReactionIfThroughWater(this);
    if (al::updateNerveState(this)) kill();
}
void KinokoOneUp::exeForceGet() {
    mForceGetSpeed = (mForceGetSpeed + -0.4f) * 0.99f;
    mForceGetHeight += mForceGetSpeed;
    al::setTrans(this, al::getActorTrans(mForceGetSensor) + sead::Vector3f(0.0f, mForceGetHeight, 0.0f));
}
