#include "MapObj/CatGull.hpp"
#include "MapObj/CatGullHolder.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "NPC/IslandHolder.hpp"
#include "MapObj/Lighthouse.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/IslandDataList.hpp"
#include "Util/DrcUtil.hpp"
namespace {
NERVE_DECL(CatGull, Wait)
NERVE_DECL(CatGull, WaitRespawn)
NERVE_DECL(CatGull, FlyReturn)
NERVE_DECL(CatGull, Turn)
NERVE_DECL(CatGull, Fly)
NERVES_MAKE_NOSTRUCT(CatGull, Wait, WaitRespawn, FlyReturn, Turn, Fly)
}
CatGull::CatGull(const char* name) : al::LiveActor(name) {}
CatGull::~CatGull() = default;
void CatGull::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvCatGullWait, 0);
    al::startAction(this, "GroundWaitA");
    al::listenStageSwitchOnStart(this, al::Functor(this, &CatGull::startFlySwitch));
    int quadrant = 0;
    al::tryGetArg(&quadrant, info, "Quadrant");
    al::tryGetArg(&mOnLighthouse, info, "OnLighthouse");
    mIslandId = quadrant ? IslandDataFunction::getIslandIDFromParam(quadrant) : mPlacementHolder->getZoneNo();
    mConnector = al::tryCreateMtxConnector(this, info);
}
void CatGull::startFlySwitch() {
    if (al::isNerve(this, &NrvCatGullFly)) return;
    int playerId = al::findNearestPlayerId(this, -1.0f);
    sead::Vector3f dir;
    if (playerId > 0) {
        dir = al::getPlayerPos(this, playerId) - al::getTrans(this);
        dir.y = 0.0f;
        if (al::normalizeOrZero(&dir)) al::calcFrontDir(&dir, this);
    } else al::calcFrontDir(&dir, this);
    startFly(dir);
}
void CatGull::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    if (mIslandId > 0 && mOnLighthouse) {
        auto* island = static_cast<IslandHolder*>(al::getSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper)->findIsland(mIslandId));
        if (island) {
            mLighthouse = island->getLighthouse();
            auto* disaster = DisasterModeController::tryGetController(this);
            if ((mLighthouse && mLighthouse->isInked()) || (disaster && disaster->isRaining())) {
                al::hideModelIfShow(this);
                al::setNerve(this, &NrvCatGullWaitRespawn);
            }
        }
        al::getSceneObj<CatGullHolder>(this, SceneObjID_CatGullHolder)->registerCatGull(this);
    }
    if (auto* disaster = DisasterModeController::tryGetController(this)) disaster->registerStateListener(this);
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    makeActorAppeared();
    mHomeTrans = al::getTrans(this);
    mClippingRadius = al::getClippingRadius(this);
}
void CatGull::pausedModelUpdate() {
    auto* disaster = DisasterModeController::tryGetController(this);
    if (disaster && disaster->isDisasterNerve()) { al::hideModelIfShow(this); return; }
    if (!al::isNerve(this, &NrvCatGullWaitRespawn)) al::showModelIfHide(this);
}
void CatGull::updateLinkedTrans(const sead::Vector3f& trans) {
    sead::Vector3f delta = trans - mHomeTrans;
    mHomeTrans = trans;
    al::setTrans(this, delta + al::getTrans(this));
}
void CatGull::startFlyReturn() {
    auto* disaster = DisasterModeController::tryGetController(this);
    if ((mLighthouse && mLighthouse->isInked()) || (disaster && disaster->isRaining()) || al::isClipped(this)) return;
    al::showModelIfHide(this);
    al::setNerve(this, &NrvCatGullFlyReturn);
}
bool CatGull::receiveMsg(const al::SensorMsg* msg, al::HitSensor* other, al::HitSensor* self) {
    if (al::isNerve(this, &NrvCatGullWait) || al::isNerve(this, &NrvCatGullTurn)) {
        if (al::isMsgPush(msg) || al::isMsgExplosion(msg) || al::isMsgPlayerFireBallAttack(msg) ||
            al::isMsgPlayerGiantHipDrop(msg) || al::isMsgNekoAttack(msg) || al::isMsgBlockUpperPunch(msg) ||
            al::isMsgItemGetAll(msg) || al::isMsgNpcTouch(msg)) {
            sead::Vector3f dir = sead::Vector3f::ez;
            al::calcDirBetweenSensorsH(&dir, other, self);
            startFly(dir);
        }
    }
    return false;
}
void CatGull::startFly(const sead::Vector3f& dir) {
    if (al::isNearZero(dir, 0.001f)) mFlightDir.set(-sead::Vector3f::ez);
    else mFlightDir.set(dir);
    if (al::isNear(mFlightDir, sead::Vector3f::ey, 0.001f)) mFlightDir.set(-sead::Vector3f::ez);
    al::makeQuatFrontUp(al::getQuatPtr(this), mFlightDir, sead::Vector3f::ey);
    mFlightDir.y = 1.0f;
    al::normalizeOrZero(&mFlightDir);
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvCatGullFly);
}
bool CatGull::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget* target) {
    if (al::isMsgTouchAssistTrig(msg) && !al::isNerve(this, &NrvCatGullFly)) {
        sead::Vector3f dir = sead::Vector3f::ez;
        const sead::Vector3f& targetPos = al::getScreenPointTargetPos(target);
        const sead::Vector3f& hitPos = al::getHitScreenPointTargetPos(pointer);
        dir.x = targetPos.x - hitPos.x;
        dir.z = targetPos.z - hitPos.z;
        dir.y = 0.0f;
        al::normalizeOrZero(&dir);
        startFly(dir);
        return true;
    }
    if (al::isMsgTouchAssist(msg) && !al::isNerve(this, &NrvCatGullFly)) {
        sead::Vector3f dir = sead::Vector3f::ez;
        auto* actor = DrcFunction::tryFindDrcTouchActor(this, pointer);
        if (!actor) return false;
        rc::tryCalcTouchPointerSlideDirOnWorldByPointer(&dir, actor);
        dir.y = 0.0f;
        al::normalizeOrZero(&dir);
        startFly(dir);
        return true;
    }
    return false;
}
void CatGull::exeWait() {
    if (al::isFirstStep(this)) {
        if (al::isHalfProbability()) al::startAction(this, "GroundWaitA");
        else al::startAction(this, "GroundWaitB");
    }
    if (al::isMicInputOn(this)) { startFlySwitch(); return; }
    if (al::isActionEnd(this)) { al::setNerve(this, &NrvCatGullTurn); return; }
    updateConnector();
}
void CatGull::updateConnector() {
    if (!mConnector) return;
    sead::Vector3f oldTrans = al::getTrans(this);
    auto* connector = mConnector;
    const auto& quat = al::getConnectBaseQuat(connector);
    const auto& baseTrans = al::getConnectBaseTrans(mConnector);
    al::connectPoseQT(this, connector, quat, oldTrans - mHomeTrans + baseTrans);
    mHomeTrans += al::getTrans(this) - oldTrans;
}
void CatGull::exeTurn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Turn");
        float angle = al::getRandom(30.0f, 90.0f);
        if (al::isHalfProbability()) angle = -angle;
        mTurnSpeed = angle / 12.0f;
    }
    if (al::isMicInputOn(this)) { startFlySwitch(); return; }
    al::rotateQuatYDirDegree(this, mTurnSpeed);
    if (al::isGreaterEqualStep(this, 12)) { al::setNerve(this, &NrvCatGullWait); return; }
    updateConnector();
}
void CatGull::exeFly() {
    if (al::isFirstStep(this)) al::startAction(this, "FlyWait");
    al::addVelocityToDirection(this, mFlightDir, 1.3f);
    al::scaleVelocity(this, 0.97f);
    mFlightDistance += al::getVelocity(this).length();
    al::setClippingInfo(this, mFlightDistance + mClippingRadius, &mHomeTrans);
    if (al::isGreaterEqualStep(this, 180)) {
        al::hideModelIfShow(this);
        al::setNerve(this, &NrvCatGullWaitRespawn);
        al::setVelocity(this, sead::Vector3f::zero);
    }
}
void CatGull::exeWaitRespawn() {
    al::isFirstStep(this);
    updateConnector();
    if (al::isGreaterEqualStep(this, 1800)) { al::validateClipping(this); startFlyReturn(); }
}
void CatGull::exeFlyReturn() {
    if (al::isFirstStep(this)) al::startAction(this, "FlyWait");
    sead::Vector3f dir = mHomeTrans - al::getTrans(this);
    dir.normalize();
    mFlightDir.set(dir);
    dir.y = 0.0f;
    updateConnector();
    if (!al::isNearZero(dir, 0.001f)) al::makeQuatFrontUp(al::getQuatPtr(this), dir, sead::Vector3f::ey);
    if (al::getTrans(this).equals(mHomeTrans, 5.0f)) {
        al::setTrans(this, mHomeTrans);
        al::setNerve(this, &NrvCatGullWait);
        al::setVelocity(this, sead::Vector3f::zero);
        al::validateHitSensors(this);
        mFlightDistance = 0.0f;
    } else {
        float distanceSquared = (al::getTrans(this) - mHomeTrans).squaredLength();
        float friction = 1.0f - 0.97f;
        float acceleration = 1.3f;
        if (distanceSquared < 2500.0f * 2500.0f) { friction *= 2.0f; acceleration /= 2.0f; }
        if (distanceSquared < 500.0f * 500.0f) { acceleration /= 1.5f; friction *= 1.5f; }
        al::addVelocityToDirection(this, mFlightDir, acceleration);
        al::scaleVelocity(this, 1.0f - friction);
        mFlightDistance -= al::getVelocity(this).length();
    }
    al::setClippingInfo(this, mFlightDistance + mClippingRadius, &mHomeTrans);
}
void CatGull::onDisasterModeStateChange(DisasterModeController::State state) {
    if (al::isClipped(this)) return;
    if (state == DisasterModeController::State::RainStart) startFlySwitch();
    else if (state == DisasterModeController::State::Prosperity && al::isHideModel(this)) startFlyReturn();
}
void CatGull::endClipped() {
    al::LiveActor::endClipped();
    auto* disaster = DisasterModeController::tryGetController(this);
    if (disaster && disaster->isRaining()) { startFlySwitch(); al::hideModelIfShow(this); return; }
    if (mOnLighthouse && (!mLighthouse || mLighthouse->isInked())) return;
    if (al::isNerve(this, &NrvCatGullWaitRespawn) && al::isGreaterEqualStep(this, 1800)) startFlyReturn();
}
