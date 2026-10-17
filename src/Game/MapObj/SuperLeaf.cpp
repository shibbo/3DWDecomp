#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/SuperLeaf.hpp"
#include "MapObj/ItemStateLeaf.hpp"
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
    ItemStatePopUpFrontParam sSuperLeafParam;
    NERVE_DECL(SuperLeaf, Wait);
    NERVE_DECL(SuperLeaf, WaitAndCheckCollision);
    NERVE_DECL(SuperLeaf, PopUpAbove);
    NERVE_DECL(SuperLeaf, PopUpFront);
    NERVE_DECL(SuperLeaf, Homing);
    NERVE_DECL(SuperLeaf, AttachBubble);
    NERVES_MAKE_STRUCT(SuperLeaf, Wait, PopUpAbove, PopUpFront, Homing, WaitAndCheckCollision, AttachBubble)
}
SuperLeaf::SuperLeaf(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
SuperLeaf::~SuperLeaf() {}
void SuperLeaf::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "SuperLeaf", nullptr);
    al::initNerve(this, &NrvSuperLeaf.Wait, 4);
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mLeaf = new ItemStateLeaf(this, &sSuperLeafParam);
    mHoming = new ItemStatePopUpFront(this);
    mCustomParam = new ItemStatePopUpFrontParam();
    mCollisionState = new ItemStateCheckCollision(this);
    al::initNerveState(this, mPopUpAbove, &NrvSuperLeaf.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mLeaf, &NrvSuperLeaf.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mHoming, &NrvSuperLeaf.Homing, "[state]Homing");
    al::initNerveState(this, mCollisionState, &NrvSuperLeaf.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvSuperLeaf.AttachBubble);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}
void SuperLeaf::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void SuperLeaf::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakeOut = false;
    if (mBubble) al::setNerve(this, &NrvSuperLeaf.AttachBubble);
    else al::setNerve(this, &NrvSuperLeaf.Wait);
}
bool SuperLeaf::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvSuperLeaf.AttachBubble)) {
        if (rc::isMsgItemBubbleBreak(msg)) {
            al::setNerve(this, &NrvSuperLeaf.PopUpFront);
            return true;
        }
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            sender = mBubble->getHitPlayerSensor();
            bool isTakeOut = mIsTakeOut;
            if (al::isSensorPlayer(sender) && !rc::isPlayerRaccoonDog(sender)) al::startHitReaction(al::getSensorHost(sender), "スーパーこのは取得");
            if (!isTakeOut) rc::addScore(this, sender, 0.0f, 0);
            rc::acquirerItemSuperLeaf(this, sender);
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
        if (al::isSensorPlayer(sender) && !rc::isPlayerRaccoonDog(sender)) al::startHitReaction(al::getSensorHost(sender), "スーパーこのは取得");
        if (!isTakeOut) rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemSuperLeaf(this, sender);
        kill();
        return true;
    }
    if (rc::tryDisappearItemByStartGoalDemoHouse(this, msg)) return true;
    if (rc::tryDisappearItemByStartGoalDemoPole(this, msg)) return true;
    if (al::isNerve(this, &NrvSuperLeaf.PopUpFront)) mLeaf->receiveMsg(msg, sender, receiver);
    if (al::isNerve(this, &NrvSuperLeaf.Homing) && mHoming->receiveMsg(msg, sender, receiver)) return true;
    if (al::isNerve(this, &NrvSuperLeaf.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool SuperLeaf::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvSuperLeaf.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void SuperLeaf::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void SuperLeaf::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::LiveActor::appear();
    al::setNerve(this, &NrvSuperLeaf.PopUpAbove);
}
void SuperLeaf::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mIsTakeOut = false;
    mLeaf->setTakeOut(false);
    if (mDisablePopUpCollision) {
        *mCustomParam = sSuperLeafParam;
        mCustomParam->_74 = false;
        mLeaf->setParam(mCustomParam);
    } else mLeaf->setParam(&sSuperLeafParam);
    al::setNerve(this, &NrvSuperLeaf.PopUpFront);
}
void SuperLeaf::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mLeaf->setTakeOut(true);
    mLeaf->setParam(&sSuperLeafParam);
    mCollisionState->enableWaterReaction();
    al::setNerve(this, &NrvSuperLeaf.PopUpFront);
}
void SuperLeaf::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) {
    *mCustomParam = *param;
    mLeaf->setParam(mCustomParam);
}
void SuperLeaf::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void SuperLeaf::exePopUpAbove() {
    al::updateNerveStateAndNextNerve(this, &NrvSuperLeaf.WaitAndCheckCollision);
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void SuperLeaf::exePopUpFront() {
    al::updateNerveStateAndNextNerve(this, &NrvSuperLeaf.WaitAndCheckCollision);
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
void SuperLeaf::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}

void SuperLeaf::appearItemHoming(const al::HitSensor* sensor) {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    mIsTakeOut = true;
    mHoming->setParamHoming(sensor);
    mHoming->setParamAnimName("TakeOut");
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(sensor))
        mHoming->setParamInvalidateKillByArea(true);
    al::setNerve(this, &NrvSuperLeaf.Homing);
}
void SuperLeaf::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvSuperLeaf.PopUpFront);
}
void SuperLeaf::exeHoming() {
    al::updateNerveStateAndNextNerve(this, &NrvSuperLeaf.WaitAndCheckCollision);
    if (mIsTakeOut) rc::startHitReactionIfThroughWater(this);
    else rc::startHitReactionDeathIfThroughWater(this);
}
