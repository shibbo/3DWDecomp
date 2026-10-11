#include "MapObj/Bird.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Util/DrcUtil.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
namespace {
    inline void setFlyDirection(sead::Vector3f* out, const sead::Vector3f& direction, bool nearZero) {
        if (nearZero) *out = -sead::Vector3f::ez;
        else out->set(direction);
    }
    NERVE_DECL(Bird, Wait);
    NERVE_DECL(Bird, Fly);
    NERVE_DECL(Bird, Turn);
    NERVES_MAKE_NOSTRUCT(Bird, Wait, Fly, Turn)
}
Bird::Bird(const char* name) : al::LiveActor(name) {}
Bird::~Bird() {}
void Bird::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvBirdWait, 0);
    al::listenStageSwitchOnStart(this, al::Functor(this, &Bird::startFlySwitch));
    makeActorAppeared();
}
void Bird::startFlySwitch() {
    if (al::isNerve(this, &NrvBirdFly)) return;
    sead::Vector3f direction;
    int player = al::findNearestPlayerId(this, -1.0f);
    if (player > 0) {
        direction = al::getPlayerPos(this, player) - al::getTrans(this);
        direction.y = 0.0f;
        if (al::normalizeOrZero(&direction)) al::calcFrontDir(&direction, this);
    } else al::calcFrontDir(&direction, this);
    startFly(direction);
}
bool Bird::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgItemGetAll(msg) || al::isMsgExplosion(msg) || al::isMsgPlayerFireBallAttack(msg) ||
        al::isMsgPlayerGiantHipDrop(msg) || al::isMsgBlockUpperPunch(msg)) {
        sead::Vector3f direction = sead::Vector3f::ez;
        al::calcDirBetweenSensorsH(&direction, sender, receiver);
        startFly(direction);
    }
    return false;
}
void Bird::startFly(const sead::Vector3f& direction) {
    bool nearZero = al::isNearZero(direction, 0.001f);
    setFlyDirection(&_148, direction, nearZero);
    if (al::isNear(_148, sead::Vector3f::ey, 0.001f)) _148 = -sead::Vector3f::ez;
    al::makeQuatFrontUp(al::getQuatPtr(this), _148, sead::Vector3f::ey);
    _148.y = 1.0f;
    al::normalizeOrZero(&_148);
    al::invalidateClipping(this);
    al::invalidateHitSensors(this);
    al::setNerve(this, &NrvBirdFly);
}
bool Bird::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget* target) {
    if (al::isMsgTouchAssistTrig(msg) && !al::isNerve(this, &NrvBirdFly)) {
        sead::Vector3f direction = sead::Vector3f::ez;
        const sead::Vector3f& targetPos = al::getScreenPointTargetPos(target);
        const sead::Vector3f& hitPos = al::getHitScreenPointTargetPos(pointer);
        direction.x = targetPos.x - hitPos.x;
        direction.z = targetPos.z - hitPos.z;
        direction.y = 0.0f;
        al::normalizeOrZero(&direction);
        startFly(direction);
        return true;
    }
    if (al::isMsgTouchAssist(msg) && !al::isNerve(this, &NrvBirdFly)) {
        sead::Vector3f direction = sead::Vector3f::ez;
        al::LiveActor* touchActor = DrcFunction::tryFindDrcTouchActor(this, pointer);
        if (!touchActor) return false;
        rc::tryCalcTouchPointerSlideDirOnWorldByPointer(&direction, touchActor);
        direction.y = 0.0f;
        al::normalizeOrZero(&direction);
        startFly(direction);
        return true;
    }
    return false;
}
void Bird::exeWait() {
    if (al::isFirstStep(this)) {
        if (al::isHalfProbability()) al::startAction(this, "GroundWaitA");
        else al::startAction(this, "GroundWaitB");
    }
    if (al::isMicInputOn(this)) {
        startFlySwitch();
        return;
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvBirdTurn);
}
void Bird::exeTurn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Turn");
        float angle = al::getRandom(30.0f, 90.0f);
        if (al::isHalfProbability()) angle = -angle;
        _144 = angle / 12.0f;
    }
    if (al::isMicInputOn(this)) {
        startFlySwitch();
        return;
    }
    al::rotateQuatYDirDegree(this, _144);
    if (al::isGreaterEqualStep(this, 12)) al::setNerve(this, &NrvBirdWait);
}
void Bird::exeFly() {
    if (al::isFirstStep(this)) al::startAction(this, "FlyWait");
    al::addVelocityToDirection(this, _148, 1.3f);
    al::scaleVelocity(this, 0.97f);
    if (al::isGreaterEqualStep(this, 180)) kill();
}
