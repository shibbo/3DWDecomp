#include "MapObj/KinokoBig.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/KinokoStateRunaway.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(KinokoBig, PopUpFront);
    NERVE_DECL(KinokoBig, Runaway);
    NERVES_MAKE_STRUCT(KinokoBig, PopUpFront, Runaway)
}
KinokoBig::KinokoBig(const char* name) : al::LiveActor(name) {}
KinokoBig::~KinokoBig() {}
void KinokoBig::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "KinokoBig", nullptr);
    al::initNerve(this, &NrvKinokoBig.PopUpFront, 2);
    mPopUpState = new ItemStatePopUpFront(this);
    mRunawayState = new KinokoStateRunaway(this);
    al::initNerveState(this, mPopUpState, &NrvKinokoBig.PopUpFront, "跳ね上げ(前方)");
    al::initNerveState(this, mRunawayState, &NrvKinokoBig.Runaway, "逃げる");
    mRunawayState->setSpeed(3.5f);
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    makeActorAppeared();
}
void KinokoBig::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvKinokoBig.PopUpFront);
}
void KinokoBig::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvKinokoBig.Runaway) && al::isSensorMapObj(receiver) && al::isSensorEye(sender) && al::isSensorName(sender, "KinokoBigBody")) al::sendMsgPushAndKillVelocityToTarget(this, sender, receiver);
}
bool KinokoBig::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvKinokoBig.Runaway) && al::isSensorEye(sender) && al::isSensorName(sender, "KinokoBigBody") && al::isSensorMapObj(receiver) && al::tryReceiveMsgPushAndAddVelocity(this, msg, sender, receiver, 1.0f)) return true;
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangReflect(msg)) {
        if (mRumble->isEnd()) {
            al::startHitReactionHit(this);
            rc::requestHitReactionToAttacker(msg, receiver, sender);
            mRumble->start(0);
        }
        return true;
    }
    if (!al::isSensorPlayer(sender)) return false;
    if (al::isMsgItemGetDirectAll(msg)) {
        rc::addScore(this, sender, 0.0f, 0);
        rc::acquirerItemKinokoBig(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}
bool KinokoBig::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) { return al::isMsgTouchAssist(msg); }
void KinokoBig::control() {
    if (!mRumble->isEnd()) {
        mRumble->calc();
        al::setScaleY(this, mRumble->getValueY() + 1.0f);
    } else al::setScaleY(this, 1.0f);
    al::updateMaterialCodeWater(this);
}
void KinokoBig::appearPopUpFront() {
    ItemStatePopUpFrontParam param;
    appearPopUpFront(param);
}
void KinokoBig::appearPopUpFront(const ItemStatePopUpFrontParam& param) {
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::hideModelIfShow(this);
    al::LiveActor::appear();
    mPopUpState->setParam(param, nullptr);
    al::setNerve(this, &NrvKinokoBig.PopUpFront);
}
void KinokoBig::exePopUpFront() {
    al::updateNerveStateAndNextNerve(this, &NrvKinokoBig.Runaway);
    rc::startHitReactionIfThroughWater(this);
}
void KinokoBig::exeRunaway() {
    al::holdSe(this, "PgRunningLvBig", nullptr);
    rc::startHitReactionIfThroughWater(this);
    if (al::updateNerveState(this)) kill();
}
