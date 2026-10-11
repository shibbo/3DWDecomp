#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/FireFlower.hpp"
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
    NERVE_DECL(FireFlower, Wait);
    NERVE_DECL(FireFlower, WaitAndCheckCollision);
    NERVE_DECL(FireFlower, PopUpAbove);
    NERVE_DECL(FireFlower, PopUpFront);
    NERVE_DECL(FireFlower, AttachBubble);
    NERVES_MAKE_STRUCT(FireFlower, Wait, WaitAndCheckCollision, PopUpAbove, PopUpFront, AttachBubble)
}
FireFlower::FireFlower(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
FireFlower::~FireFlower() {}
void FireFlower::init(const al::ActorInitInfo& info) {
    if (mBubble) al::initActorWithArchiveName(this, info, "FireFlower", "Attach");
    else al::initActorWithArchiveName(this, info, "FireFlower", nullptr);
    al::initNerve(this, &NrvFireFlower.Wait, 3);
    mCollisionState = new ItemStateCheckCollision(this);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mPopUpFront = new ItemStatePopUpFront(this);
    al::initNerveState(this, mCollisionState, &NrvFireFlower.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    al::initNerveState(this, mPopUpAbove, &NrvFireFlower.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mPopUpFront, &NrvFireFlower.PopUpFront, "[state]跳ね上げ(前方)");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvFireFlower.AttachBubble);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}
void FireFlower::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void FireFlower::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    if (mBubble) al::setNerve(this, &NrvFireFlower.AttachBubble);
    else al::setNerve(this, &NrvFireFlower.Wait);
}
bool FireFlower::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvFireFlower.AttachBubble) || al::isNerve(this, &NrvFireFlower.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvFireFlower.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvFireFlower.AttachBubble)) {
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            sender = mBubble->getHitPlayerSensor();
            if (!mIsTakeOut) rc::addScore(this, sender, 0.0f, 0);
            rc::acquirerItemFireFlower(this, sender);
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
        rc::acquirerItemFireFlower(this, sender);
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvFireFlower.PopUpFront) && mPopUpFront->receiveMsg(msg, sender, receiver)) return true;
    if (al::isNerve(this, &NrvFireFlower.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool FireFlower::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvFireFlower.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void FireFlower::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void FireFlower::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    al::setNerve(this, &NrvFireFlower.PopUpAbove);
}
void FireFlower::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    mPopUpFront->setParamDefault();
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvFireFlower.PopUpFront);
}
void FireFlower::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    mCollisionState->enableWaterReaction();
    al::setNerve(this, &NrvFireFlower.PopUpFront);
}
void FireFlower::appearItemHoming(const al::HitSensor* target) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamHoming(target);
    mPopUpFront->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(target)) mPopUpFront->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvFireFlower.PopUpFront);
}
void FireFlower::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void FireFlower::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void FireFlower::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvFireFlower.PopUpFront);
}
void FireFlower::exePopUpAbove() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvFireFlower.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void FireFlower::exePopUpFront() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvFireFlower.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void FireFlower::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}
