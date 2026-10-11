#include "Enemy/KillerStateFly.hpp"
#include "Enemy/Killer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"

struct KillerStateFlyParam {
    int stepSearch;
    int stepCollide;
    float turnSpeed;
    float bankAngleRange;
    float bankAngleMax;
    float bankSpeed;
};

namespace {
NERVE_DECL(KillerStateFly, FlyWaitStart)
class KillerStateFlyNrvFlySearch : public al::Nerve {
public:
    /** @brief Updates the homing projectile's flight.
     * @param pKeeper Flight-state nerve keeper.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KillerStateFly>()->exeFlyWait();
    }
};
NERVE_DECL(KillerStateFly, FlyWait)
NERVES_MAKE_NOSTRUCT(KillerStateFly, FlyWaitStart, FlySearch, FlyWait)
const KillerStateFlyParam cNormalParam = {20, 20, 1.0f, 35.0f, 35.0f, 1.0f};
const KillerStateFlyParam cMagnumParam = {57, 60, 0.4f, 40.0f, 30.0f, 0.4f};

/** @brief Sets velocity along an actor's forward direction.
 * @param pActor Projectile whose velocity is updated.
 * @param speed Forward movement speed.
 */
void setVelocityFront(al::LiveActor* pActor, float speed) {
    sead::Vector3f front(0.0f, 0.0f, 0.0f);
    al::calcFrontDir(&front, pActor);
    al::setVelocityToDirection(pActor, front, speed);
}
}

/** @brief Constructs the Bullet Bill flight state.
 * @param pHost Projectile controlled by this state.
 */
KillerStateFly::KillerStateFly(Killer* pHost)
    : al::NerveStateBase("キラーの飛行状態"), mHost(pHost) {
    initNerve(&NrvKillerStateFlyFlyWaitStart, 0);
}

/** @brief Starts or resumes flight, restoring collision after an interruption. */
void KillerStateFly::appear() {
    al::NerveStateBase::appear();
    mHost->validateAttackSensors();
    if (mStepFly > 0) {
        mHost->tryOnCollide();
        if (Killer::isSearch(mHost->mType)) {
            al::setNerve(this, &NrvKillerStateFlyFlySearch);
        } else {
            al::setNerve(this, &NrvKillerStateFlyFlyWait);
        }
    } else {
        al::setNerve(this, &NrvKillerStateFlyFlyWaitStart);
    }
}

/** @brief Saves elapsed flight time and marks the state inactive. */
void KillerStateFly::kill() {
    mStepFly += al::getNerveStep(this);
    al::NerveStateBase::kill();
}

/** @brief Clears elapsed flight time before the projectile is reused. */
void KillerStateFly::reset() {
    mStepFly = 0;
}

/** @brief Plays the launch animation and selects straight or homing flight. */
void KillerStateFly::exeFlyWaitStart() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "FlyWaitStart");
        setVelocityFront(mHost, mHost->getAccel());
    }
    if (al::isActionEnd(mHost)) {
        if (Killer::isSearch(mHost->mType)) {
            al::setNerve(this, &NrvKillerStateFlyFlySearch);
        } else {
            al::setNerve(this, &NrvKillerStateFlyFlyWait);
        }
    }
}

/** @brief Updates steering, banking, collision activation, and flight lifetime. */
void KillerStateFly::exeFlyWait() {
    if (al::isFirstStep(this)) {
        al::startAction(mHost, "FlyWait");
    }
    if (al::isNerve(this, &NrvKillerStateFlyFlySearch) &&
        rc::calcActivePlayerNum(mHost) > 0 &&
        al::isGreaterEqualStep(this, getParam().stepSearch)) {
        al::LiveActor* pPlayer = rc::findNearestActivePlayerActor(mHost);
        float angle = al::calcAngleToTargetH(mHost, al::getTrans(pPlayer));
        float turnSpeed = getParam().turnSpeed;
        float turnAngle = sead::Mathf::clamp(angle, -turnSpeed, turnSpeed);
        al::rotateQuatYDirDegree(mHost, turnAngle);
        float bankRate = al::normalizeAbs(angle, 0.0f, getParam().bankAngleRange);
        float& rJointRotation = mHost->mJointRotation;
        rJointRotation = al::converge(rJointRotation,
                                            -bankRate * getParam().bankAngleMax,
                                            getParam().bankSpeed);
    }
    setVelocityFront(mHost, mHost->getAccel());
    Killer* pHost = mHost;
    int collideStep = static_cast<int>(getParam().stepCollide * pHost->getAccelRate());
    if (getStepFly() == collideStep) {
        mHost->tryOnCollide();
        al::validateHitSensors(mHost);
    }
    if ((al::isCollided(mHost) && (mStepFly > 0 || getStepFly() >= collideStep)) ||
        mHost->getStepDisappear() <= getStepFly()) {
        kill();
    }
}

/** @brief Gets elapsed flight time, excluding the launch animation.
 * @return Elapsed flight steps, or zero while inactive or launching.
 */
int KillerStateFly::getStepFly() const {
    if (mIsDead || al::isNerve(this, &NrvKillerStateFlyFlyWaitStart)) {
        return 0;
    }
    return al::getNerveStep(this) + mStepFly;
}

/** @brief Selects normal or Magnum flight parameters.
 * @return Parameters corresponding to the host projectile's size.
 */
inline const KillerStateFlyParam& KillerStateFly::getParam() const {
    return Killer::isMagnum(mHost->mType) ? cMagnumParam : cNormalParam;
}
