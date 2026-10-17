#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/WhiteBell.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStateCheckCollision.hpp"
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
    __attribute__((noinline)) void acquireWhiteBell(al::LiveActor* actor, al::HitSensor* sensor, bool isTakeOut) {
        if (al::isSensorPlayer(sensor) && !rc::isPlayerClimb(sensor))
            al::startHitReaction(al::getSensorHost(sensor), "HrWhiteBellCollect");
        if (!isTakeOut) rc::addScore(actor, sensor, 0.0f, 0);
        al::startHitReactionGet(actor);
        rc::acquirerItemWhiteBell(actor, sensor);
        actor->kill();
    }
    NERVE_DECL(WhiteBell, Wait);
    NERVE_DECL(WhiteBell, WaitAndCheckCollision);
    NERVE_DECL(WhiteBell, PopUpFront);
    NERVE_DECL(WhiteBell, AttachBubble);
    NERVES_MAKE_STRUCT(WhiteBell, Wait, PopUpFront, WaitAndCheckCollision, AttachBubble)
}
WhiteBell::WhiteBell(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
WhiteBell::~WhiteBell() {}
void WhiteBell::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "WhiteBell", nullptr);
    al::initNerve(this, &NrvWhiteBell.Wait, 2);
    mPopUpFront = new ItemStatePopUpFront(this);
    mCollisionState = new ItemStateCheckCollision(this);
    al::initNerveState(this, mPopUpFront, &NrvWhiteBell.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mCollisionState, &NrvWhiteBell.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvWhiteBell.AttachBubble);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}
void WhiteBell::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void WhiteBell::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    if (mBubble) al::setNerve(this, &NrvWhiteBell.AttachBubble);
    else al::setNerve(this, &NrvWhiteBell.Wait);
}
bool WhiteBell::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvWhiteBell.AttachBubble) || al::isNerve(this, &NrvWhiteBell.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvWhiteBell.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvWhiteBell.AttachBubble)) {
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            acquireWhiteBell(this, mBubble->getHitPlayerSensor(), mIsTakeOut);
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
        acquireWhiteBell(this, sender, mIsTakeOut);
        return true;
    }
    if (al::isNerve(this, &NrvWhiteBell.PopUpFront)) mPopUpFront->receiveMsg(msg, sender, receiver);
    if (al::isNerve(this, &NrvWhiteBell.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool WhiteBell::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvWhiteBell.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void WhiteBell::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void WhiteBell::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    mPopUpFront->setParamDefault();
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvWhiteBell.PopUpFront);
}
void WhiteBell::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    mCollisionState->enableWaterReaction();
    al::setNerve(this, &NrvWhiteBell.PopUpFront);
}
void WhiteBell::appearItemHoming(const al::HitSensor* target) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mPopUpFront->setParamHoming(target);
    mPopUpFront->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(target)) mPopUpFront->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvWhiteBell.PopUpFront);
}
void WhiteBell::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void WhiteBell::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void WhiteBell::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvWhiteBell.PopUpFront);
}
void WhiteBell::exePopUpFront() {
    if (al::updateNerveState(this)) { al::setNerve(this, &NrvWhiteBell.WaitAndCheckCollision); return; }
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void WhiteBell::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}
