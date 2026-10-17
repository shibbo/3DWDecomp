#include "MapObj/KinokoSuper.hpp"
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
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(KinokoSuper, Wait);
NERVE_DECL(KinokoSuper, PopUpFront);
NERVE_DECL(KinokoSuper, PopUpAbove);
NERVE_DECL(KinokoSuper, Runaway);
NERVE_DECL(KinokoSuper, AttachBubble);
NERVE_DECL(KinokoSuper, Land);
NERVES_MAKE_STRUCT(KinokoSuper, Wait, PopUpFront, PopUpAbove, Runaway, AttachBubble, Land)
}
KinokoSuper::KinokoSuper(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
KinokoSuper::~KinokoSuper() {}
void KinokoSuper::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "KinokoSuper", nullptr);
    al::initNerve(this, &NrvKinokoSuper.Wait, 3);
    mPopUpFront = new ItemStatePopUpFront(this);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mRunaway = new KinokoStateRunaway(this);
    al::initNerveState(this, mPopUpFront, &NrvKinokoSuper.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mPopUpAbove, &NrvKinokoSuper.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mRunaway, &NrvKinokoSuper.Runaway, "[state]逃げる");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvKinokoSuper.AttachBubble);
    bool connect = false;
    al::tryGetArg(&connect, info, "IsConnectToCollision");
    if (connect) mConnector = al::createMtxConnector(this);
    mColliderRadius = al::getColliderRadius(this);
    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &KinokoSuper::appearPopUpFront))) makeActorDead();
    else makeActorAppeared();
}
void KinokoSuper::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    mIsTakenOut = false;
    mCanRunaway = true;
    mPopUpFront->setParamDefault();
    if (mCollideOnPopUp) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvKinokoSuper.PopUpFront);
}
void KinokoSuper::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    al::updateMaterialCodeArea(this);
}
void KinokoSuper::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakenOut = false;
    mCanRunaway = true;
    if (mBubble) al::setNerve(this, &NrvKinokoSuper.AttachBubble);
    else al::setNerve(this, &NrvKinokoSuper.Wait);
}
bool KinokoSuper::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    sead::Vector3f direction(sead::Vector3f::zero);
    if (rc::tryGetItemReflectHitDir(&direction, msg)) {
        direction.y = 0.0f;
        if (!al::normalizeOrZero(&direction) && al::getVelocity(this).dot(direction) < 0.0f) {
            al::setVelocity(this, al::calcSpeed(this) * direction);
            return true;
        }
    }
    if ((al::isNerve(this, &NrvKinokoSuper.AttachBubble) || al::isNerve(this, &NrvKinokoSuper.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvKinokoSuper.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvKinokoSuper.AttachBubble)) {
        if (!rc::isMsgItemBubbleBreakAndGetItem(msg)) return false;
        if (!mBubble->isEnableGetPlayerSensor()) return false;
        sender = mBubble->getHitPlayerSensor();
        if (!mIsTakenOut) rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemKinokoSuper(this, sender);
        kill();
        return true;
    }
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg)) {
        if (mRumble->isEnd()) {
            al::startHitReactionHit(this);
            rc::requestHitReactionToAttacker(msg, receiver, sender);
            mRumble->start(0);
        }
        return al::isMsgPlayerFireBallAttack(msg);
    }
    if (rc::tryDisappearItemByStartGoalDemoHouse(this, msg) || rc::tryDisappearItemByStartGoalDemoPole(this, msg)) return true;
    if (al::isMsgItemGetDirectAll(msg)) {
        if (!mIsTakenOut) rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemKinokoSuper(this, sender);
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvKinokoSuper.PopUpFront) && mPopUpFront->receiveMsg(msg, sender, receiver)) return true;
    if (al::isNerve(this, &NrvKinokoSuper.Runaway)) mRunaway->receiveMsg(msg, sender, receiver);
    return false;
}

bool KinokoSuper::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvKinokoSuper.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void KinokoSuper::control() {
    if (!mRumble->isEnd()) { mRumble->calc(); al::setScaleY(this, mRumble->getValueY() + 1.0f); }
    else al::setScaleY(this, 1.0f);
    al::updateMaterialCodeWater(this);
}
void KinokoSuper::appearPopUpFrontNoRunaway() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    mIsTakenOut = false;
    mCanRunaway = false;
    mPopUpFront->setParamDefault();
    al::setNerve(this, &NrvKinokoSuper.PopUpFront);
}
void KinokoSuper::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    mIsTakenOut = false;
    mCanRunaway = true;
    al::setNerve(this, &NrvKinokoSuper.PopUpAbove);
}
void KinokoSuper::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakenOut = true;
    mCanRunaway = true;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    al::setNerve(this, &NrvKinokoSuper.PopUpFront);
}
void KinokoSuper::appearItemHoming(const al::HitSensor* sensor) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakenOut = true;
    mCanRunaway = true;
    mPopUpFront->setParamHoming(sensor);
    mPopUpFront->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(sensor)) mPopUpFront->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvKinokoSuper.PopUpFront);
}
void KinokoSuper::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void KinokoSuper::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (mConnector) al::connectPoseQT(this, mConnector);
}
void KinokoSuper::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvKinokoSuper.PopUpFront);
}
void KinokoSuper::exePopUpFront() {
    if (al::updateNerveState(this)) {
        if (mCanRunaway) al::setNerve(this, &NrvKinokoSuper.Runaway);
        else al::setNerve(this, &NrvKinokoSuper.Land);
    }
    if (mIsTakenOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void KinokoSuper::exePopUpAbove() {
    if (al::updateNerveState(this)) {
        if (mCanRunaway) al::setNerve(this, &NrvKinokoSuper.Runaway);
        else al::setNerve(this, &NrvKinokoSuper.Land);
    }
    if (mIsTakenOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void KinokoSuper::exeRunaway() {
    if (mIsTakenOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    if (al::isFirstStep(this)) al::startSe(this, "Land", nullptr);
    al::holdSe(this, "PgRunning", nullptr);
    if (mIsTakenOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
    if (al::updateNerveState(this)) kill();
}
void KinokoSuper::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
        al::startSe(this, "Land", nullptr);
        al::setVelocityZero(this);
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKinokoSuper.Wait);
}
