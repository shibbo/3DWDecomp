#include "Boss/PunpunShuriken.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
namespace {
NERVE_DECL(PunpunShuriken, Wait);
NERVE_DECL(PunpunShuriken, Appear);
NERVE_DECL(PunpunShuriken, DemoAppear);
NERVE_DECL(PunpunShuriken, Shoot);
class PunpunShurikenNrvShootLv2 : public al::Nerve {
public: void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<PunpunShuriken>()->exeShoot(); }
};
NERVES_MAKE_NOSTRUCT(PunpunShuriken, Wait, Appear, DemoAppear)
NERVES_MAKE_STRUCT(PunpunShuriken, Shoot, ShootLv2)
const sead::Vector3f sShootOffset(0.0f, 30.0f, 50.0f);
}
/** @brief Creates a throwable shuriken. @param pName Actor name. */
PunpunShuriken::PunpunShuriken(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the shuriken. */
PunpunShuriken::~PunpunShuriken() = default;
/** @brief Loads the standard shuriken archive. @param rInfo Initialization data. */
void PunpunShuriken::init(const al::ActorInitInfo& rInfo) { initShurikenWithArchiveName(rInfo, "PunpunShuriken"); }
/** @brief Loads an archive and leaves the shuriken inactive. @param rInfo Initialization data. @param pArchiveName Model archive. */
void PunpunShuriken::initShurikenWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchiveName) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString(pArchiveName), nullptr);
    al::initNerve(this, &NrvPunpunShurikenWait, 0);
    makeActorDead();
}
/** @brief Starts the hidden appearance animation. */
void PunpunShuriken::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvPunpunShurikenAppear);
    al::hideModelIfShow(this);
}
/** @brief Starts the demo appearance animation. */
void PunpunShuriken::appearDemo() { appear(); al::setNerve(this, &NrvPunpunShurikenDemoAppear); }
/** @brief Plays the disappearance reaction and kills the actor. */
void PunpunShuriken::kill() { al::startHitReactionDisappear(this); al::LiveActor::kill(); }
/** @brief Attacks overlapping sensors during a normal throw. @param pSelf Attack sensor. @param pOther Contacted sensor. */
void PunpunShuriken::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf) && al::isNerve(this, &NrvPunpunShuriken.Shoot) &&
        al::isHitCircleSensor(pOther, pSelf, sead::Vector3f::ey, al::getSensorRadius(pSelf), 3.0f) &&
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) kill();
}
/** @brief Handles attacks against a normally thrown shuriken. @param pMsg Incoming message. @param pOther Sender sensor. @param pSelf Body sensor. @return Whether handled. */
bool PunpunShuriken::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isSensorEnemyBody(pSelf)) return false;
    if (!al::isNerve(this, &NrvPunpunShuriken.Shoot)) return false;
    if (EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) || EnemyStateUtil::isMsgBlowDown(pMsg)) {
        kill(); return true;
    }
    return false;
}
/** @brief Launches the shuriken horizontally. @param rDir Throw direction. @param isLv2 Whether to home toward players. */
void PunpunShuriken::shoot(const sead::Vector3f& rDir, bool isLv2) {
    sead::Vector3f dir = rDir;
    dir.y = 0.0f;
    al::normalize(&dir);
    sead::Matrix34f mtx(sead::Matrix34f::ident);
    al::makeMtxFrontUpPos(&mtx, dir, sead::Vector3f::ey, al::getTrans(this));
    al::updatePoseMtx(this, &mtx);
    al::calcTransLocalOffset(al::getTransPtr(this), this, sShootOffset);
    al::faceToDirection(this, dir);
    al::setVelocityToDirection(this, dir, 15.0f);
    al::setNerve(this, isLv2 ? static_cast<const al::Nerve*>(&NrvPunpunShuriken.ShootLv2) : &NrvPunpunShuriken.Shoot);
}
/** @brief Shows the model during its demo appearance. */
void PunpunShuriken::exeDemoAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "DemoAppear");
    if (al::isStep(this, 1)) al::showModelIfHide(this);
    al::setNerveAtActionEnd(this, &NrvPunpunShurikenWait);
}
/** @brief Shows the model during its normal appearance. */
void PunpunShuriken::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear");
    if (al::isStep(this, 1)) al::showModelIfHide(this);
    al::setNerveAtActionEnd(this, &NrvPunpunShurikenWait);
}
/** @brief Starts the held idle animation. */
void PunpunShuriken::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
/** @brief Checks collision and lifetime, then steers homing throws. */
void PunpunShuriken::exeShoot() {
    if (al::isFirstStep(this)) al::startAction(this, "Spin");
    if (al::isCollided(this) || al::isGreaterEqualStep(this, 300)) { kill(); return; }
    if (al::isNerve(this, &NrvPunpunShuriken.ShootLv2) && rc::calcActivePlayerNum(this) > 0 && al::isGreaterEqualStep(this, 15)) {
        float angle = al::calcAngleToTargetH(this, al::findNearestPlayerPos(this));
        al::rotateQuatYDirDegree(this, sead::Mathf::clamp(angle, -1.0f, 1.0f));
        sead::Vector3f front(0.0f, 0.0f, 0.0f);
        al::calcFrontDir(&front, this);
        al::setVelocityToDirection(this, front, 15.0f);
    }
}
