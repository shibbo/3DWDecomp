#include "MapObj/KinokoTreasure.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
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
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
namespace {
NERVE_DECL(KinokoTreasure, Wait);
NERVE_DECL(KinokoTreasure, PopUpFront);
NERVE_DECL(KinokoTreasure, PopUpAbove);
NERVE_DECL(KinokoTreasure, Runaway);
NERVE_DECL(KinokoTreasure, AttachBubble);
NERVE_DECL(KinokoTreasure, Land);
NERVES_MAKE_STRUCT(KinokoTreasure, Wait, PopUpFront, Runaway, AttachBubble, Land)
NERVES_MAKE_NOSTRUCT(KinokoTreasure, PopUpAbove)
}
KinokoTreasure::KinokoTreasure(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
KinokoTreasure::~KinokoTreasure() {}
void KinokoTreasure::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "KinokoTreasure", nullptr);
    al::initNerve(this, &NrvKinokoTreasure.Wait, 2);
    mPopUpFront = new ItemStatePopUpFront(this);
    mRunaway = new KinokoStateRunaway(this);
    al::initNerveState(this, mPopUpFront, &NrvKinokoTreasure.PopUpFront, "[state]跳ね上げ(前方)");
    al::initNerveState(this, mRunaway, &NrvKinokoTreasure.Runaway, "[state]逃げる");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    mFlashing = new al::FlashingCtrl(this, true, false);
    if (mBubble) al::setNerve(this, &NrvKinokoTreasure.AttachBubble);
    al::tryGetArg(&mConnectToCollision, info, "IsConnectToCollision");
    mConnector = al::createMtxConnector(this);
    mColliderRadius = al::getColliderRadius(this);
    al::listenStageSwitchOnKill(this, al::FunctorV0M(this, &KinokoTreasure::kill));
    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &KinokoTreasure::appearPopUpFront))) makeActorDead();
    else makeActorAppeared();
}
void KinokoTreasure::appearPopUpFront() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    mIsTakenOut = false;
    mCanRunaway = true;
    mAttachDuringPopUp = false;
    mPopUpFront->setParamDefault();
    al::setNerve(this, &NrvKinokoTreasure.PopUpFront);
}
void KinokoTreasure::initAfterPlacement() {
    if (mConnectToCollision) al::attachMtxConnectorToCollision(mConnector, this, false);
    if (getEffectKeeper()) al::updateMaterialCodeArea(this);
}
void KinokoTreasure::appear() {
    al::setColliderRadius(this, mColliderRadius);
    al::LiveActor::appear();
    mIsTakenOut = false;
    mCanRunaway = true;
    if (mBubble) al::setNerve(this, &NrvKinokoTreasure.AttachBubble);
    else al::setNerve(this, &NrvKinokoTreasure.Wait);
}
bool KinokoTreasure::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgRestore(msg)) { appear(); return true; }
    sead::Vector3f direction(sead::Vector3f::zero);
    if (rc::tryGetItemReflectHitDir(&direction, msg)) {
        direction.y = 0.0f;
        if (!al::normalizeOrZero(&direction) && al::getVelocity(this).dot(direction) < 0.0f) {
            al::setVelocity(this, al::calcSpeed(this) * direction);
            return true;
        }
    }
    if ((al::isNerve(this, &NrvKinokoTreasure.AttachBubble) || al::isNerve(this, &NrvKinokoTreasure.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        mPopUpFront->setParamDefault();
        al::setNerve(this, &NrvKinokoTreasure.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvKinokoTreasure.AttachBubble)) {
        if (!rc::isMsgItemBubbleBreakAndGetItem(msg)) return false;
        if (!mBubble->isEnableGetPlayerSensor()) return false;
        sender = mBubble->getHitPlayerSensor();
        rc::acquirerItemCoin50(this, sender);
        al::startHitReactionGet(this);
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
    if (al::isMsgItemGetDirectAll(msg) || (al::isSensorHoldObj(sender) && al::isMsgBallItemGet(msg))) {
        al::startHitReactionGet(this);
        rc::acquirerItemCoin50(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvKinokoTreasure.PopUpFront)) mPopUpFront->receiveMsg(msg, sender, receiver);
    if (al::isNerve(this, &NrvKinokoTreasure.Runaway)) mRunaway->receiveMsg(msg, sender, receiver);
    return false;
}

bool KinokoTreasure::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvKinokoTreasure.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void KinokoTreasure::control() {
    if (!mRumble->isEnd()) { mRumble->calc(); al::setScaleY(this, mRumble->getValueY() + 1.0f); }
    else al::setScaleY(this, 1.0f);
    al::updateMaterialCodeWater(this);
}
void KinokoTreasure::appearPopUpFrontNoRunaway() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    mIsTakenOut = false;
    mCanRunaway = false;
    mAttachDuringPopUp = false;
    mPopUpFront->setParamDefault();
    al::setNerve(this, &NrvKinokoTreasure.PopUpFront);
}
void KinokoTreasure::appearPopUpAbove() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    al::startAction(this, "PopUp");
    alLiveActorFunction::calcAnimDirect(this);
    mIsTakenOut = false;
    mCanRunaway = false;
    mAttachDuringPopUp = false;
    al::setNerve(this, &NrvKinokoTreasurePopUpAbove);
}
void KinokoTreasure::appearItemTakeOut() {
    al::setColliderRadius(this, mColliderRadius);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::startSe(this, "PgAppear", nullptr);
    al::LiveActor::appear();
    mIsTakenOut = true;
    mCanRunaway = true;
    mAttachDuringPopUp = false;
    mPopUpFront->setParamDefault();
    mPopUpFront->setParamAnimName("TakeOut");
    al::setNerve(this, &NrvKinokoTreasure.PopUpFront);
}
void KinokoTreasure::appearPopUpAboveConnectToCollision() {
    appearPopUpAbove();
    mAttachDuringPopUp = true;
    mConnector->setBaseQuatTrans(al::getQuat(this), al::getTrans(this));
    al::attachMtxConnectorToCollision(mConnector, this, false);
}
void KinokoTreasure::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void KinokoTreasure::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::offCollide(this);
        if (mValidateClippingOnWait) al::validateClipping(this);
    }
    if (mConnectToCollision || mAttachDuringPopUp) al::connectPoseQT(this, mConnector);
}
void KinokoTreasure::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvKinokoTreasure.PopUpFront);
}
void KinokoTreasure::exePopUpFront() {
    if (al::updateNerveState(this)) {
        if (mCanRunaway) al::setNerve(this, &NrvKinokoTreasure.Runaway);
        else al::setNerve(this, &NrvKinokoTreasure.Land);
    }
    rc::startHitReactionIfThroughWater(this);
}
void KinokoTreasure::exePopUpAbove() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::invalidateHitSensors(this);
        al::faceToTarget(this, al::getCameraPos_RS(this, 0));
    }
    if (mAttachDuringPopUp) al::connectPoseQT(this, mConnector);
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this)) return;
    if (al::isStep(this, 40)) al::validateHitSensors(this);
    if (al::isActionEnd(this)) {
        if (mCanRunaway) al::setNerve(this, &NrvKinokoTreasure.Runaway);
        else al::setNerve(this, &NrvKinokoTreasure.Wait);
    }
    rc::startHitReactionIfThroughWater(this);
}
void KinokoTreasure::exeRunaway() {
    if (mIsTakenOut) {
        if (al::isFirstStep(this)) mFlashing->start(600);
        mFlashing->movement();
        if (mFlashing->isEnded()) { al::validateClipping(this); kill(); return; }
    }
    if (al::isFirstStep(this)) al::startSe(this, "Land", nullptr);
    al::holdSe(this, "PgRunning", nullptr);
    rc::startHitReactionIfThroughWater(this);
    if (al::updateNerveState(this)) kill();
}
void KinokoTreasure::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
        al::startSe(this, "Land", nullptr);
        al::setVelocityZero(this);
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKinokoTreasure.Wait);
}

void KinokoTreasure::kill() { al::LiveActor::kill(); }
