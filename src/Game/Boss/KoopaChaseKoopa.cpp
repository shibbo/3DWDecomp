#include "Boss/KoopaChaseKoopa.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"

namespace {
NERVE_DECL(KoopaChaseKoopa, Stop);
NERVE_DECL(KoopaChaseKoopa, Damage);
NERVE_DECL(KoopaChaseKoopa, DamageSmall);
NERVE_DECL(KoopaChaseKoopa, Wait);
NERVE_DECL(KoopaChaseKoopa, WarpProvocation);
NERVES_MAKE_NOSTRUCT(KoopaChaseKoopa, Stop, Damage, DamageSmall, Wait, WarpProvocation)
ActorJointLookControllerParam sLookLeft(1.5f, sead::Vector2f(-3.0f, 9.5f), false, nullptr, nullptr);
ActorJointLookControllerParam sLookRight(1.5f, sead::Vector2f(-9.5f, 3.0f), false, nullptr, nullptr);
ActorJointLookControllerParam sLookVertical(0.5f, sead::Vector2f(1.0f, 8.0f), true, nullptr, nullptr);
}

/** @brief Creates Bowser's chase model. @param pHost Host actor, unused by the original constructor. */
KoopaChaseKoopa::KoopaChaseKoopa(al::LiveActor* pHost) : al::LiveActor("クッパ") {}
/** @brief Destroys the chase model. */
KoopaChaseKoopa::~KoopaChaseKoopa() = default;

/** @brief Initializes the model, eye look controllers, and spring joints. @param rInfo Actor initialization data. */
void KoopaChaseKoopa::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Koopa", nullptr);
    al::initNerve(this, &NrvKoopaChaseKoopaStop, 0);
    al::initJointControllerKeeper(this, 21);
    al::initJointGlobalYRotator(this, &mSpineRotation, "Spine");
    mLookController = new ActorJointLookController(this, 4);
    mLookController->appendJoint("EyeL", sead::Vector3f::ey, &sLookLeft);
    mLookController->appendJoint("EyeL", -sead::Vector3f::ez, &sLookVertical);
    mLookController->appendJoint("EyeR", sead::Vector3f::ey, &sLookRight);
    mLookController->appendJoint("EyeR", -sead::Vector3f::ez, &sLookVertical);
    al::initJointSpringController(this, "HairBack1");
    al::initJointSpringController(this, "HairBack2");
    al::initJointSpringController(this, "HairBack3");
    al::initJointSpringController(this, "HairBack4");
    al::initJointSpringController(this, "HairFront1");
    al::initJointSpringController(this, "HairFront2");
    al::initJointSpringController(this, "HairFront3");
    al::initJointSpringController(this, "HairL1");
    al::initJointSpringController(this, "HairL2");
    al::initJointSpringController(this, "HairL3");
    al::initJointSpringController(this, "HairR1");
    al::initJointSpringController(this, "HairR2");
    al::initJointSpringController(this, "HairR3");
    al::initJointSpringController(this, "HairMiddle1");
    al::initJointSpringController(this, "HairMiddle2");
    al::initJointSpringController(this, "HairMiddle3");
    makeActorAppeared();
}

/** @brief Accepts touch-assist messages. @param pMsg Incoming message. @param pPointer Screen pointer. @param pTarget Screen target. @return Whether this is touch assist. */
bool KoopaChaseKoopa::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}
/** @brief Starts full damage. */
void KoopaChaseKoopa::doDamage() { al::setNerve(this, &NrvKoopaChaseKoopaDamage); }
/** @brief Starts small damage. */
void KoopaChaseKoopa::doDamageSmall() { al::setNerve(this, &NrvKoopaChaseKoopaDamageSmall); }
/** @brief Starts the waiting state. */
void KoopaChaseKoopa::setStateWait() { al::setNerve(this, &NrvKoopaChaseKoopaWait); }
/** @brief Starts the warp taunt. */
void KoopaChaseKoopa::setStateWarpProvocation() { al::setNerve(this, &NrvKoopaChaseKoopaWarpProvocation); }
/** @brief Tests either damage state. @return Whether Bowser is reacting to damage. */
bool KoopaChaseKoopa::isStateDamage() const { return al::isNerve(this, &NrvKoopaChaseKoopaDamage) || al::isNerve(this, &NrvKoopaChaseKoopaDamageSmall); }
/** @brief Tests the warp taunt state. @return Whether the taunt is active. */
bool KoopaChaseKoopa::isStateWarpProvocation() const { return al::isNerve(this, &NrvKoopaChaseKoopaWarpProvocation); }

/** @brief Turns the spine and eyes toward the target. @param rTarget World-space look target. */
void KoopaChaseKoopa::updatePose(const sead::Vector3f& rTarget) {
    sead::Vector3f delta = rTarget - al::getTrans(this);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    front.y = 0.0f;
    float rotation = (delta.x * front.z - delta.z * front.x) * 0.05f;
    if (!(sead::Mathf::abs(mSpineRotation - rotation) > 90.0f)) {
        rotation = mSpineRotation + sead::Mathf::clamp(rotation - mSpineRotation, -0.7f, 0.7f);
    }
    mSpineRotation = rotation;
    mLookController->setLookTarget(rTarget);
    mLookController->update();
}
/** @brief Holds the model until the host starts it. */
void KoopaChaseKoopa::exeStop() {}
/** @brief Starts the waiting animation. */
void KoopaChaseKoopa::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
/** @brief Plays Damage and returns to waiting. */
void KoopaChaseKoopa::exeDamage() {
    if (al::isFirstStep(this)) al::startAction(this, "Damage");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaChaseKoopaWait);
}
/** @brief Plays DamageSmall and returns to waiting. */
void KoopaChaseKoopa::exeDamageSmall() {
    if (al::isFirstStep(this)) al::startAction(this, "DamageSmall");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaChaseKoopaWait);
}
/** @brief Plays WarpProvocation and returns to waiting. */
void KoopaChaseKoopa::exeWarpProvocation() {
    if (al::isFirstStep(this)) al::startAction(this, "WarpProvocation");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaChaseKoopaWait);
}
