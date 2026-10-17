#include "Enemy/RingBeamer.hpp"
#include "Enemy/RingBeamerBeam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
NERVE_DECL(RingBeamer, Active)
NERVE_DECL(RingBeamer, BeginActive)
NERVE_DECL(RingBeamer, Interval)
NERVE_DECL(RingBeamer, EndInterval)
NERVE_DECL(RingBeamer, EmitBeam)
NERVE_DECL(RingBeamer, Deactive)
NERVE_DECL(RingBeamer, BeginDeactive)
NERVE_DECL(RingBeamer, Stop)
NERVES_MAKE_NOSTRUCT(RingBeamer, Active, BeginActive, Interval, EndInterval, EmitBeam, Deactive, BeginDeactive, Stop)
}
/** @brief Creates the emitter. @param pName Actor name. */
RingBeamer::RingBeamer(const char* pName) : al::LiveActor(pName) { mParam = new RingBeamerParam(); }
/** @brief Creates the beam and connects the stop switch. @param rInfo Placement information. */
void RingBeamer::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "RingBeamer", nullptr);
    al::initNerve(this, &NrvRingBeamerActive, 0);
    mConnector = al::createMtxConnector(this);
    al::tryGetArg(&mParam->mSpeed, rInfo, "BeamSpeed");
    al::tryGetArg(&mParam->mLifeTime, rInfo, "BeamLife");
    al::tryGetArg(&mInterval, rInfo, "BeamInterval");
    const char* pObjectName = nullptr;
    if (!al::tryGetObjectName(&pObjectName, rInfo)) { kill(); return; }
    RingBeamerType type = RingBeamerType_Circle;
    if (al::isEqualSubString(pObjectName, "Square")) type = RingBeamerType_Square;
    else if (al::isEqualSubString(pObjectName, "Triangle")) type = static_cast<RingBeamerType>(2);
    else if (al::isEqualSubString(pObjectName, "Star")) type = static_cast<RingBeamerType>(3);
    al::initSubActorKeeperNoFile(this, rInfo, 1);
    mBeam = new RingBeamerBeam("\u30ea\u30f3\u30b0\u30d3\u30fc\u30e0", this, type, true);
    al::initCreateActorNoPlacementInfo(mBeam, rInfo);
    al::registerSubActorSyncClipping(this, mBeam, false);
    mBeam->setRingBeamerParam(mParam);
    makeActorAppeared();
    al::trySetShadowLength(this, rInfo, nullptr);
    mSignFrames = al::getSklAnimFrameMax(this, "Sign");
    al::listenStageSwitchOnStop(this, al::Functor(this, &RingBeamer::stop));
}
/** @brief Stops the current emission cycle. */
void RingBeamer::stop() { al::setNerve(this, &NrvRingBeamerStop); }
/** @brief Attaches to the supporting collision. */
void RingBeamer::initAfterPlacement() { al::attachMtxConnectorToCollision(mConnector, this, false); }
/** @brief Leaves damage handling to the beam. @param pSelf Own sensor. @param pOther Contact sensor. */
void RingBeamer::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {}
/** @brief Deactivates while touched. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool RingBeamer::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerFloorTouch(pMsg) || al::isMsgTouchAssist(pMsg)) {
        mCooldown = 15;
        if (isActive()) al::setNerve(this, &NrvRingBeamerBeginDeactive);
        return true;
    }
    return false;
}
/** @brief Checks whether emission is enabled. @return Whether in an active state. */
bool RingBeamer::isActive() const {
    return al::isNerve(this, &NrvRingBeamerActive) || al::isNerve(this, &NrvRingBeamerBeginActive) || al::isNerve(this, &NrvRingBeamerInterval) || al::isNerve(this, &NrvRingBeamerEndInterval) || al::isNerve(this, &NrvRingBeamerEmitBeam);
}
/** @brief Follows the supporting collision. */
void RingBeamer::control() { al::connectPoseQT(this, mConnector); }
/** @brief Waits for the previous beam to disappear. */
void RingBeamer::exeActive() {
    if (al::isFirstStep(this)) al::startAction(this, "WaitActive");
    if (!mBeam->isActive() && !mBeam->isFadeOut()) {
        al::validateClipping(this);
        al::setNerve(this, &NrvRingBeamerInterval);
    }
}
/** @brief Restarts emission after the release animation. */
void RingBeamer::exeBeginActive() {
    if (al::isFirstStep(this)) al::startAction(this, "RideOff");
    if (al::isActionEnd(this) && al::isGreaterEqualStep(this, 60)) al::setNerve(this, &NrvRingBeamerInterval);
}
/** @brief Waits between beams. */
void RingBeamer::exeInterval() {
    al::isFirstStep(this);
    if (al::isGreaterEqualStep(this, static_cast<s32>(static_cast<f32>(mInterval) - mSignFrames))) al::setNerve(this, &NrvRingBeamerEndInterval);
}
/** @brief Plays the warning animation. */
void RingBeamer::exeEndInterval() {
    if (al::isFirstStep(this)) al::startAction(this, "Sign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvRingBeamerEmitBeam);
}
/** @brief Launches a new beam. */
void RingBeamer::exeEmitBeam() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "Launch");
        mBeam->setActive();
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvRingBeamerActive);
}
/** @brief Counts down until the emitter can restart. */
void RingBeamer::exeDeactive() {
    if (al::isFirstStep(this)) { al::validateClipping(this); al::startAction(this, "WaitDeactive"); }
    if (mCooldown-- <= 0) al::setNerve(this, &NrvRingBeamerBeginActive);
}
/** @brief Plays the depression animation. */
void RingBeamer::exeBeginDeactive() {
    if (al::isFirstStep(this)) al::startAction(this, "RideOn");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvRingBeamerDeactive);
}
/** @brief Cancels the current beam and deactivates. */
void RingBeamer::exeStop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RideOn");
        if (mBeam->isActive()) {
            mBeam->setDeactive();
            if (getEffectKeeper()) al::tryDeleteEmitterAndParticleAll(this);
        }
        al::setNerve(this, &NrvRingBeamerDeactive);
    }
}
