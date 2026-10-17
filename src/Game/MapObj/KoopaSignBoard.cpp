#include "MapObj/KoopaSignBoard.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(KoopaSignBoard, Wait);
NERVE_DECL(KoopaSignBoard, BreakFront);
NERVE_DECL(KoopaSignBoard, BreakBack);
NERVE_DECL(KoopaSignBoard, BreakSignBack);
NERVE_DECL(KoopaSignBoard, BreakSignFront);
NERVE_DECL(KoopaSignBoard, BreakAttacked);
NERVE_DECL(KoopaSignBoard, BreakEndFront);
NERVE_DECL(KoopaSignBoard, BreakEndBack);
NERVE_DECL(KoopaSignBoard, BreakEnd);
NERVES_MAKE_STRUCT(KoopaSignBoard, Wait, BreakFront, BreakBack, BreakSignBack, BreakSignFront, BreakAttacked)
NERVES_MAKE_NOSTRUCT(KoopaSignBoard, BreakEndFront, BreakEndBack, BreakEnd)
}
KoopaSignBoard::KoopaSignBoard(const char* name) : al::LiveActor(name) {}
KoopaSignBoard::~KoopaSignBoard() {}
void KoopaSignBoard::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvKoopaSignBoard.Wait, 0);
    mBreakModel = new al::LiveActor("クッパ看板壊れ");
    al::initActorWithArchiveName(mBreakModel, info, "KoopaSignBoardBreak", nullptr);
    mBreakModel->kill();
    float scale = al::getScaleX(this);
    if (scale > 1.0f) {
        al::setSensorRadius(this, "Body", scale * al::getSensorRadius(this, "Body"));
        al::setSensorRadius(this, "Arm", scale * al::getSensorRadius(this, "Arm"));
        al::setSensorRadius(this, "Head", scale * al::getSensorRadius(this, "Head"));
        al::setSensorRadius(this, "Top", scale * al::getSensorRadius(this, "Top"));
        al::setSensorFollowPosOffset(this, "Body", scale * al::getSensorFollowPosOffset(this, "Body"));
        al::setSensorFollowPosOffset(this, "Top", scale * al::getSensorFollowPosOffset(this, "Top"));
    }
    makeActorAppeared();
}
void KoopaSignBoard::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorEnemyBody(receiver) && (al::isNerve(this, &NrvKoopaSignBoard.BreakFront) || al::isNerve(this, &NrvKoopaSignBoard.BreakBack))) al::sendMsgEnemyAttackFire(receiver, sender);
}
bool KoopaSignBoard::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (!al::isNerve(this, &NrvKoopaSignBoard.Wait) && !al::isNerve(this, &NrvKoopaSignBoard.BreakSignBack) && !al::isNerve(this, &NrvKoopaSignBoard.BreakSignFront)) return false;
    if (al::isMsgPlayerTouch(msg) && !al::isNerve(this, &NrvKoopaSignBoard.BreakSignBack) && !al::isNerve(this, &NrvKoopaSignBoard.BreakSignFront)) {
        if (isTouchFront(sender)) al::setNerve(this, &NrvKoopaSignBoard.BreakSignBack);
        else al::setNerve(this, &NrvKoopaSignBoard.BreakSignFront);
        setScoreSensor(msg, sender);
        return true;
    }
    if (al::isMsgPlayerRollingAttack(msg) || al::isMsgPlayerHipDropAll(msg)) {
        al::setNerve(this, &NrvKoopaSignBoard.BreakAttacked);
        rc::addScoreCombo(this, sender, msg, 100.0f);
        return true;
    }
    if (al::isNerve(this, &NrvKoopaSignBoard.Wait) && al::isMsgTouchAssistTrig(msg)) {
        al::setNerve(this, &NrvKoopaSignBoard.BreakSignBack);
        if (auto* player = DrcFunction::tryFindDrcPlayerSensor(this, sender)) setScoreSensor(msg, player);
        return true;
    }
    if (al::isMsgPlayerInvincibleAttack(msg) ||
        al::isMsgPlayerSlidingAttack(msg) ||
        al::isMsgPlayerSpinAttack(msg) ||
        al::isMsgExplosion(msg) ||
        al::isMsgPlayerTailAttack(msg) ||
        al::isMsgPlayerClimbAttack(msg) ||
        al::isMsgPlayerClimbRollingAttack(msg) ||
        al::isMsgPlayerBoomerangReflect(msg) ||
        al::isMsgPlayerBodyAttack(msg) ||
        al::isMsgPlayerClimbSlidingAttack(msg) ||
        al::isMsgPlayerFireBallAttack(msg) || rc::isMsgPackunPush(msg)) {
        if (sead::Mathf::abs(al::getTrans(this).z - al::getSensorPos(sender).z) - 40.0f <= al::getSensorRadius(sender)) {
            al::setNerve(this, &NrvKoopaSignBoard.BreakAttacked);
            if (!rc::isMsgPackunPush(msg)) rc::addScoreCombo(this, sender, msg, 100.0f);
            return true;
        }
    }
    return false;
}
bool KoopaSignBoard::isTouchFront(const al::HitSensor* sensor) const {
    return al::calcAngleDegree(al::getFront(this), al::getSensorPos(sensor) - al::getTrans(this)) < 90.0f;
}
void KoopaSignBoard::setScoreSensor(const al::SensorMsg*, al::HitSensor* sensor) { mScoreSensor = sensor; }
void KoopaSignBoard::addScoreToSensor() { if (mScoreSensor) rc::addScore(this, mScoreSensor, 100.0f, 0); }
void KoopaSignBoard::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
void KoopaSignBoard::exeBreakSignFront() {
    if (al::isFirstStep(this)) al::startAction(this, "BreakFrontSign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaSignBoard.BreakFront);
}
void KoopaSignBoard::exeBreakFront() {
    if (al::isFirstStep(this)) { al::startAction(this, "BreakFront"); al::invalidateCollisionParts(this); }
    if (al::isActionEnd(this)) {
        mBreakModel->appear();
        addScoreToSensor();
        al::startAction(mBreakModel, "BreakFront");
        al::setNerve(this, &NrvKoopaSignBoardBreakEndFront);
    }
}
void KoopaSignBoard::exeBreakSignBack() {
    if (al::isFirstStep(this)) al::startAction(this, "BreakBackSign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaSignBoard.BreakBack);
}
void KoopaSignBoard::exeBreakBack() {
    if (al::isFirstStep(this)) { al::startAction(this, "BreakBack"); al::invalidateCollisionParts(this); }
    if (al::isActionEnd(this)) {
        mBreakModel->appear();
        addScoreToSensor();
        al::startAction(mBreakModel, "BreakBack");
        al::setNerve(this, &NrvKoopaSignBoardBreakEndBack);
    }
}
void KoopaSignBoard::exeBreakAttacked() {
    if (al::isFirstStep(this)) {
        al::invalidateCollisionParts(this);
        al::hideModelIfShow(this);
        mBreakModel->appear();
        al::startAction(mBreakModel, "BreakAttack");
        al::setNerve(this, &NrvKoopaSignBoardBreakEnd);
    }
}
void KoopaSignBoard::exeBreakEnd() {
    if (al::isFirstStep(this)) al::hideModelIfShow(this);
    if (al::isActionEnd(mBreakModel)) {
        al::startAction(mBreakModel, "Wait");
        al::tryOnStageSwitch(this, "SwitchBreakOn");
        kill();
    }
}
void KoopaSignBoard::exeBreakEndFront() {
    if (al::isFirstStep(this)) al::hideModelIfShow(this);
    if (al::isActionEnd(mBreakModel)) {
        al::startAction(mBreakModel, "WaitFront");
        al::tryOnStageSwitch(this, "SwitchBreakOn");
        kill();
    }
}
void KoopaSignBoard::exeBreakEndBack() {
    if (al::isFirstStep(this)) al::hideModelIfShow(this);
    if (al::isActionEnd(mBreakModel)) {
        al::startAction(mBreakModel, "WaitBack");
        al::tryOnStageSwitch(this, "SwitchBreakOn");
        kill();
    }
}
