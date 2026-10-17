#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "MapObj/AssistLeaf.hpp"
#include "MapObj/ItemStateLeaf.hpp"
#include "System/GameDataHolderWriter.hpp"
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
    ItemStatePopUpFrontParam sAssistLeafParam;
    NERVE_DECL(AssistLeaf, Wait);
    NERVE_DECL(AssistLeaf, WaitAndCheckCollision);
    NERVE_DECL(AssistLeaf, PopUpAbove);
    NERVE_DECL(AssistLeaf, PopUpFront);
    NERVES_MAKE_STRUCT(AssistLeaf, Wait, PopUpAbove, PopUpFront, WaitAndCheckCollision)
}
AssistLeaf::AssistLeaf(const char* name) : al::LiveActor(name) {}
AssistLeaf::~AssistLeaf() {}
void AssistLeaf::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "AssistLeaf", nullptr);
    al::initNerve(this, &NrvAssistLeaf.Wait, 3);
    sAssistLeafParam.setDefault();
    mPopUpAbove = new ItemStatePopUpAbove(this);
    mLeaf = new ItemStateLeaf(this, &sAssistLeafParam);
    mCustomParam = new ItemStatePopUpFrontParam();
    mCollisionState = new ItemStateCheckCollision(this);
    al::initNerveState(this, mPopUpAbove, &NrvAssistLeaf.PopUpAbove, "[state]跳ね上げ(真上)");
    al::initNerveState(this, mLeaf, &NrvAssistLeaf.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mCollisionState, &NrvAssistLeaf.WaitAndCheckCollision, "[state]着地&コリジョンチェック");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    mColliderRadius = al::getColliderRadius(this);
    makeActorAppeared();
}
void AssistLeaf::initAfterPlacement() { al::updateMaterialCodeArea(this); }
void AssistLeaf::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
}
bool AssistLeaf::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg)) {
        if (mRumble->isEnd()) {
            al::startHitReactionHit(this);
            rc::requestHitReactionToAttacker(msg, receiver, sender);
            mRumble->start(0);
        }
        return al::isMsgPlayerFireBallAttack(msg);
    }
    if (al::isMsgItemGetDirectAll(msg)) {
        if (!mIsTakeOut) rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemAssistLeaf(this, sender);
        GameDataFunction::setUseAssistBlock(GameDataHolderWriter(this));
        if (al::isSensorPlayer(sender) && !rc::isPlayerRaccoonDogWhite(sender) && (al::isSingleMode(this) || !rc::isPlayerClimbWhite(sender)))
            al::startHitReaction(al::getSensorHost(sender), "無敵このは取得");
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    if (rc::tryDisappearItemByStartGoalDemoHouse(this, msg) || rc::tryDisappearItemByStartGoalDemoPole(this, msg)) return true;
    if (al::isNerve(this, &NrvAssistLeaf.PopUpFront)) mLeaf->receiveMsg(msg, sender, receiver);
    if (al::isNerve(this, &NrvAssistLeaf.WaitAndCheckCollision)) mCollisionState->receiveMsg(msg, sender, receiver);
    return false;
}
bool AssistLeaf::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    return al::isMsgTouchAssist(msg);
}
void AssistLeaf::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
}
void AssistLeaf::appearPopUpAbove() {
    mIsTakeOut = false;
    al::invalidateHitSensors(this);
    appear();
    al::setNerve(this, &NrvAssistLeaf.PopUpAbove);
}
void AssistLeaf::appearPopUpFront() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    appear();
    mIsTakeOut = false;
    mLeaf->setTakeOut(false);
    mLeaf->setParam(&sAssistLeafParam);
    al::setNerve(this, &NrvAssistLeaf.PopUpFront);
}
void AssistLeaf::appearItemTakeOut() {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    appear();
    mIsTakeOut = true;
    mLeaf->setTakeOut(true);
    mLeaf->setParam(&sAssistLeafParam);
    al::setNerve(this, &NrvAssistLeaf.PopUpFront);
}
void AssistLeaf::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) {
    *mCustomParam = *param;
    mLeaf->setParam(mCustomParam);
}
void AssistLeaf::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
}
void AssistLeaf::exePopUpAbove() {
    al::updateNerveStateAndNextNerve(this, &NrvAssistLeaf.WaitAndCheckCollision);
    rc::startHitReactionIfThroughWater(this);
}
void AssistLeaf::exePopUpFront() {
    al::updateNerveStateAndNextNerve(this, &NrvAssistLeaf.WaitAndCheckCollision);
    rc::startHitReactionIfThroughWater(this);
}
void AssistLeaf::exeWaitAndCheckCollision() {
    if (mIsTakeOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    al::updateNerveState(this);
}
