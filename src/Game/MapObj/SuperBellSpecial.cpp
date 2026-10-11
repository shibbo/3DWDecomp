#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/SuperBellSpecial.hpp"
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
    NERVE_DECL(SuperBellSpecial, Wait);
    NERVE_DECL(SuperBellSpecial, WaitAndCheckCollision);
    NERVE_DECL(SuperBellSpecial, PopUpAbove);
    NERVE_DECL(SuperBellSpecial, PopUpFront);
    NERVE_DECL(SuperBellSpecial, AttachBubble);
    NERVES_MAKE_STRUCT(SuperBellSpecial, Wait, PopUpFront, PopUpAbove, WaitAndCheckCollision, AttachBubble)
}
SuperBellSpecial::SuperBellSpecial(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
SuperBellSpecial::~SuperBellSpecial() {}
void SuperBellSpecial::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "SuperBellSpecial", nullptr);
    al::initNerve(this, &NrvSuperBellSpecial.Wait, 3);
    mPopUpFront = new ItemStatePopUpFront(this);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mCollisionState = new ItemStateCheckCollision(this);
    al::initNerveState(this, mPopUpFront, &NrvSuperBellSpecial.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mPopUpAbove, &NrvSuperBellSpecial.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mCollisionState, &NrvSuperBellSpecial.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvSuperBellSpecial.AttachBubble);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}
void SuperBellSpecial::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void SuperBellSpecial::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    if (mBubble) al::setNerve(this, &NrvSuperBellSpecial.AttachBubble);
    else al::setNerve(this, &NrvSuperBellSpecial.Wait);
}
bool SuperBellSpecial::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvSuperBellSpecial.AttachBubble) || al::isNerve(this, &NrvSuperBellSpecial.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvSuperBellSpecial.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvSuperBellSpecial.AttachBubble)) {
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            sender = mBubble->getHitPlayerSensor();
            if (!mIsTakeOut) rc::addScore(this, sender, 0.0f, 0);
            rc::acquirerItemSuperBellSpecial(this, sender);
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
        rc::acquirerItemSuperBellSpecial(this, sender);
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvSuperBellSpecial.PopUpFront) && mPopUpFront->receiveMsg(msg, sender, receiver)) return true;
    if (al::isNerve(this, &NrvSuperBellSpecial.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool SuperBellSpecial::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvSuperBellSpecial.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void SuperBellSpecial::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void SuperBellSpecial::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    al::setNerve(this, &NrvSuperBellSpecial.PopUpAbove);
}
void SuperBellSpecial::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    mPopUpFront->setParamDefault();
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvSuperBellSpecial.PopUpFront);
}
void SuperBellSpecial::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    mCollisionState->enableWaterReaction();
    al::setNerve(this, &NrvSuperBellSpecial.PopUpFront);
}
void SuperBellSpecial::appearItemHoming(const al::HitSensor* target) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamHoming(target);
    mPopUpFront->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(target)) mPopUpFront->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvSuperBellSpecial.PopUpFront);
}
void SuperBellSpecial::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void SuperBellSpecial::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void SuperBellSpecial::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvSuperBellSpecial.PopUpFront);
}
void SuperBellSpecial::exePopUpAbove() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvSuperBellSpecial.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void SuperBellSpecial::exePopUpFront() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvSuperBellSpecial.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void SuperBellSpecial::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}
