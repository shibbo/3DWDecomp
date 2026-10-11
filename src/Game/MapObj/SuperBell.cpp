#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/SuperBell.hpp"
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
    NERVE_DECL(SuperBell, Wait);
    NERVE_DECL(SuperBell, WaitAndCheckCollision);
    NERVE_DECL(SuperBell, PopUpAbove);
    NERVE_DECL(SuperBell, PopUpFront);
    NERVE_DECL(SuperBell, AttachBubble);
    NERVES_MAKE_STRUCT(SuperBell, Wait, PopUpFront, PopUpAbove, WaitAndCheckCollision, AttachBubble)
}
SuperBell::SuperBell(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
SuperBell::~SuperBell() {}
void SuperBell::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "SuperBell", nullptr);
    al::initNerve(this, &NrvSuperBell.Wait, 3);
    mPopUpFront = new ItemStatePopUpFront(this);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mCollisionState = new ItemStateCheckCollision(this);
    al::initNerveState(this, mPopUpFront, &NrvSuperBell.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mPopUpAbove, &NrvSuperBell.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mCollisionState, &NrvSuperBell.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvSuperBell.AttachBubble);
    makeActorAppeared();
    mColliderRadius = al::getColliderRadius(this);
}
void SuperBell::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void SuperBell::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    if (mBubble) al::setNerve(this, &NrvSuperBell.AttachBubble);
    else al::setNerve(this, &NrvSuperBell.Wait);
}
bool SuperBell::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvSuperBell.AttachBubble) || al::isNerve(this, &NrvSuperBell.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvSuperBell.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvSuperBell.AttachBubble)) {
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            sender = mBubble->getHitPlayerSensor();
            bool isTakeOut = mIsTakeOut;
            if (al::isSensorPlayer(sender) && !rc::isPlayerClimb(sender)) al::startHitReaction(al::getSensorHost(sender), "クライムキノコ取得");
            if (!isTakeOut) rc::addScore(this, sender, 0.0f, 0);
            rc::acquirerItemSuperBell(this, sender);
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
    if (al::isMsgItemGetDirectAll(msg)) {
        bool isTakeOut = mIsTakeOut;
        if (al::isSensorPlayer(sender) && !rc::isPlayerClimb(sender)) al::startHitReaction(al::getSensorHost(sender), "クライムキノコ取得");
        if (!isTakeOut) rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemSuperBell(this, sender);
        kill();
        return true;
    }
    if (rc::tryDisappearItemByStartGoalDemoHouse(this, msg)) return true;
    if (rc::tryDisappearItemByStartGoalDemoPole(this, msg)) return true;
    if (al::isNerve(this, &NrvSuperBell.PopUpFront) && mPopUpFront->receiveMsg(msg, sender, receiver)) return true;
    if (al::isNerve(this, &NrvSuperBell.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool SuperBell::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvSuperBell.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void SuperBell::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void SuperBell::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    al::setNerve(this, &NrvSuperBell.PopUpAbove);
}
void SuperBell::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    mPopUpFront->setParamDefault();
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvSuperBell.PopUpFront);
}
void SuperBell::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    mCollisionState->enableWaterReaction();
    al::setNerve(this, &NrvSuperBell.PopUpFront);
}
void SuperBell::appearItemHoming(const al::HitSensor* target) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamHoming(target);
    mPopUpFront->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(target)) mPopUpFront->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvSuperBell.PopUpFront);
}
void SuperBell::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void SuperBell::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void SuperBell::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvSuperBell.PopUpFront);
}
void SuperBell::exePopUpAbove() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvSuperBell.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void SuperBell::exePopUpFront() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvSuperBell.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void SuperBell::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}
