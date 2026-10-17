#include "Enemy/PackunFlowerHead.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(PackunFlowerHead, Wait)
NERVE_DECL(PackunFlowerHead, EatGo)
NERVE_DECL(PackunFlowerHead, Eat)
NERVE_DECL(PackunFlowerHead, EatBack)
NERVES_MAKE_NOSTRUCT(PackunFlowerHead, Wait, EatGo, Eat, EatBack)
}

/** @brief Creates a biting Piranha Plant head. @param pName Actor name. */
PackunFlowerHead::PackunFlowerHead(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the head and hides it until a bite starts. @param rInfo Placement information. */
void PackunFlowerHead::init(const al::ActorInitInfo& rInfo) {
    if (rInfo.mActorSceneInfo.isSingleMode)
        al::initActorWithArchiveName(this, rInfo, "PackunFlowerHeadFur", nullptr);
    else
        al::initActorWithArchiveName(this, rInfo, "PackunFlowerHead", nullptr);
    al::initNerve(this, &NrvPackunFlowerHeadWait, 0);
    makeActorAppeared();
    al::hideModel(this);
    al::hideSilhouetteModel(this);
}

/** @brief Starts a bite from the host joint position. */
void PackunFlowerHead::start() {
    sead::Vector3f trans;
    mFollowMtx->getTranslation(trans);
    al::setTrans(this, trans);
    al::invalidateClipping(this);
    al::showSilhouetteModel(this);
    al::setNerve(this, &NrvPackunFlowerHeadEatGo);
}

/** @brief Checks whether another bite can begin. @return Whether the head is waiting. */
bool PackunFlowerHead::isEnableAppear() {
    return al::isNerve(this, &NrvPackunFlowerHeadWait);
}

/** @brief Hides the head between bites. */
void PackunFlowerHead::exeWait() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::hideSilhouetteModel(this);
    }
}

/** @brief Extends toward the target and sends the eating message. */
void PackunFlowerHead::exeEatGo() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::startAction(this, "EatGo");
    }
    sead::Vector3f start = mFollowMtx->getTranslation();
    f32 rate = static_cast<f32>(al::getNerveStep(this) + 1) / 5.0f;
    sead::Vector3f trans = start * (1.0f - rate) + rate * al::getSensorPos(mTargetSensor);
    al::setTrans(this, trans);
    sead::Vector3f direction = al::getSensorPos(mTargetSensor) - start;
    al::normalize(&direction);
    sead::Quatf quat;
    al::makeQuatFrontNoSupport(&quat, direction);
    al::setQuat(this, quat);
    if (al::isStep(this, 3))
        al::startAction(this, "Eat");
    if (al::isGreaterEqualStep(this, 4)) {
        if (rc::sendMsgPackunEat(mTargetSensor, mHolderSensor))
            al::stopScene(this, 2, 0, false, false);
        al::setNerve(this, &NrvPackunFlowerHeadEat);
    }
}

/** @brief Pauses after the bite before retracting. */
void PackunFlowerHead::exeEat() {
    if (al::isGreaterEqualStep(this, 10))
        al::setNerve(this, &NrvPackunFlowerHeadEatBack);
}

/** @brief Retracts from the target to the host joint. */
void PackunFlowerHead::exeEatBack() {
    if (al::isFirstStep(this))
        al::startAction(this, "EatBack");
    sead::Vector3f start = mFollowMtx->getTranslation();
    f32 rate = static_cast<f32>(al::getNerveStep(this) + 1) / 5.0f;
    const sead::Vector3f& rTarget = al::getSensorPos(mTargetSensor);
    f32 backRate = 1.0f - rate;
    sead::Vector3f trans = backRate * rTarget + start * rate;
    al::setTrans(this, trans);
    if (al::isGreaterEqualStep(this, 4)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvPackunFlowerHeadWait);
    }
}
