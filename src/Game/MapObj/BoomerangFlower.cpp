#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/BoomerangFlower.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStateCheckCollision.hpp"
#include "MapObj/ItemStatePopUpAbove.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(BoomerangFlower, Wait);
    NERVE_DECL(BoomerangFlower, WaitAndCheckCollision);
    NERVE_DECL(BoomerangFlower, PopUpAbove);
    NERVE_DECL(BoomerangFlower, PopUpFront);
    NERVE_DECL(BoomerangFlower, AttachBubble);
    NERVES_MAKE_STRUCT(BoomerangFlower, Wait, PopUpAbove, PopUpFront, WaitAndCheckCollision, AttachBubble)
}
BoomerangFlower::BoomerangFlower(const char* name, ItemBubble* bubble, bool attach) : al::LiveActor(name), mBubble(bubble), mIsAttach(attach) {}
BoomerangFlower::~BoomerangFlower() {}
void BoomerangFlower::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "BoomerangFlower", mIsAttach ? "Attach" : nullptr);
    al::initNerve(this, &NrvBoomerangFlower.Wait, 3);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mPopUpFront = new ItemStatePopUpFront(this);
    mCollisionState = new ItemStateCheckCollision(this);
    al::initNerveState(this, mPopUpAbove, &NrvBoomerangFlower.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mPopUpFront, &NrvBoomerangFlower.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mCollisionState, &NrvBoomerangFlower.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvBoomerangFlower.AttachBubble);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}
void BoomerangFlower::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void BoomerangFlower::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    if (mBubble) al::setNerve(this, &NrvBoomerangFlower.AttachBubble);
    else al::setNerve(this, &NrvBoomerangFlower.Wait);
}
bool BoomerangFlower::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvBoomerangFlower.AttachBubble) || al::isNerve(this, &NrvBoomerangFlower.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvBoomerangFlower.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvBoomerangFlower.AttachBubble)) {
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            sender = mBubble->getHitPlayerSensor();
            if (!mIsTakeOut) rc::addScore(this, sender, 0.0f, 0);
            rc::acquirerItemBoomerangFlower(this, sender);
            kill();
            return true;
        }
        return false;
    }
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg)) {
        if (mRumble->isEnd()) {
            al::startHitReactionHit(this);
            rc::requestHitReactionToAttacker(msg, receiver, sender);
            mRumble->start(0);
        }
        return al::isMsgPlayerFireBallAttack(msg);
    }
    if (rc::tryDisappearItemByStartGoalDemoHouse(this, msg)) return true;
    if (rc::tryDisappearItemByStartGoalDemoPole(this, msg)) return true;
    if (al::isMsgItemGetDirectAll(msg)) {
        if (!mIsTakeOut) rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemBoomerangFlower(this, sender);
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvBoomerangFlower.PopUpFront) && mPopUpFront->receiveMsg(msg, sender, receiver)) return true;
    if (al::isNerve(this, &NrvBoomerangFlower.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool BoomerangFlower::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvBoomerangFlower.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void BoomerangFlower::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void BoomerangFlower::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    al::setNerve(this, &NrvBoomerangFlower.PopUpAbove);
}
void BoomerangFlower::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    mPopUpFront->setParamDefault();
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvBoomerangFlower.PopUpFront);
}
void BoomerangFlower::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    mCollisionState->enableWaterReaction();
    al::setNerve(this, &NrvBoomerangFlower.PopUpFront);
}
void BoomerangFlower::appearItemHoming(const al::HitSensor* target) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamHoming(target);
    mPopUpFront->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(target)) mPopUpFront->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvBoomerangFlower.PopUpFront);
}
void BoomerangFlower::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void BoomerangFlower::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void BoomerangFlower::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "WaitBubble"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvBoomerangFlower.PopUpFront);
}
void BoomerangFlower::exePopUpAbove() {
    al::updateNerveStateAndNextNerve(this, &NrvBoomerangFlower.WaitAndCheckCollision);
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void BoomerangFlower::exePopUpFront() {
    al::updateNerveStateAndNextNerve(this, &NrvBoomerangFlower.WaitAndCheckCollision);
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void BoomerangFlower::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}
