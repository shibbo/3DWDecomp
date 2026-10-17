#include "MapObj/TouchReactionMapParts.hpp"
#include "Scene/ProjectActorFactoryLightActors.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_ACTION_IMPL(TouchReactionMapParts, Wait)
NERVE_ACTION_IMPL(TouchReactionMapParts, Touched)
NERVE_ACTION_IMPL(TouchReactionMapParts, Reaction)
NERVE_ACTION_IMPL(TouchReactionMapParts, Finish)
NERVE_ACTIONS_MAKE_STRUCT(TouchReactionMapParts, Wait, Touched, Reaction, Finish)
sead::Vector3f sItemAppearOffset(0.0f, 170.0f, 0.0f);
}
TouchReactionMapParts::TouchReactionMapParts(const char*) : al::LiveActor("TouchReactionMapParts") {}
TouchReactionMapParts::~TouchReactionMapParts() {}
void TouchReactionMapParts::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvTouchReactionMapParts.collector, 0);
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, info, nullptr, 0);
    mIsInvisible = getModelKeeper() == nullptr;
    if (mIsInvisible) {
        sead::Vector3f offset = al::getSensorFollowPosOffset(this, "Broadcast");
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        sead::Quatf quat;
        al::makeQuatUpNoSupport(&quat, up);
        offset.rotate(quat);
        al::setSensorFollowPosOffset(this, "Broadcast", offset);
    }
    if (al::isEffectExist(this, "Sparkle")) {
        mHasSparkle = true;
        mLight = new al::PrePassPointLight("ライトプリパス【ポイントライト】");
        al::initCreateActorWithPlacementInfo(mLight, info);
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        mLightOffset = up * 50.0f;
        al::setTrans(mLight, al::getTrans(mLight) + mLightOffset);
    }
    const char* itemType = "Dummy";
    if (al::tryGetStringArg(&itemType, info, "ItemType")) {
        mItemType = rc::getItemType(info);
        initItemKeeper(1);
        rc::addItemByHostInfo(this, info, "通常アイテム", nullptr);
    }
    al::tryGetArg(&mAnimType, info, "AnimType");
    mConnector = al::tryCreateMtxConnector(this, info);
    makeActorAppeared();
}
void TouchReactionMapParts::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    auto* controller = DisasterModeController::tryGetController(this);
    if (controller) {
        controller->registerStateListener(this);
        if (controller->getState() == DisasterModeController::State::DisasterStart) mIsDisaster = true;
    }
}
void TouchReactionMapParts::kill() {
    if (mLight && !al::isDead(mLight)) mLight->kill();
    al::tryKillEmitterAndParticleAll(this);
    al::LiveActor::kill();
}
void TouchReactionMapParts::updateLinkedTrans(const sead::Vector3f& trans) {
    mLinkedTransUpdated = true;
    al::LiveActor::updateLinkedTrans(trans);
    if (mHasSparkle) mLight->updateLinkedTrans(trans + mLightOffset);
}
void TouchReactionMapParts::control() {
    if (mConnector) {
        al::connectPoseQT(this, mConnector);
        if (mLight) al::setTrans(mLight, al::getTrans(this));
    }
    al::LiveActor::control();
}
void TouchReactionMapParts::attackSensor(al::HitSensor* self, al::HitSensor* other) {
    if (al::isNerve(this, NrvTouchReactionMapParts.Touched.data())) rc::sendMsgGraffiti(other, self, mAnimType);
    if (al::isSensorKoopaJr(other)) static_cast<PlayerKoopaJr*>(al::getSensorHost(other))->setTouchGraffitiSensor(self);
}
bool TouchReactionMapParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (al::isNerve(this, NrvTouchReactionMapParts.Touched.data()) || al::isNerve(this, NrvTouchReactionMapParts.Wait.data())) {
        if (rc::isMsgGraffiti(msg)) { al::startNerveAction(this, "Reaction"); return true; }
        return false;
    }
    return false;
}
bool TouchReactionMapParts::receiveMsgScreenPointSM(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (!al::isNerve(this, NrvTouchReactionMapParts.Reaction.data()) && !al::isNerve(this, NrvTouchReactionMapParts.Finish.data())) {
        if (!al::isNerve(this, NrvTouchReactionMapParts.Touched.data())) al::startNerveAction(this, "Touched");
        mTouchFrames = 600;
    }
    return false;
}
bool TouchReactionMapParts::hideActor() {
    if (mLight && !al::isDead(mLight)) mLight->kill();
    al::tryKillEmitterAndParticleAll(this);
    return al::LiveActor::hideActor();
}
bool TouchReactionMapParts::showActor() { return al::LiveActor::showActor(); }
void TouchReactionMapParts::startClipped() {
    if (mLight && !al::isDead(mLight)) mLight->kill();
    al::deleteEffectAll(this);
    al::LiveActor::startClipped();
}
void TouchReactionMapParts::endClipped() { al::LiveActor::endClipped(); }
void TouchReactionMapParts::respawn() {
    if (al::isDead(this)) makeActorAppeared();
    al::startNerveAction(this, "Wait");
    mTouchFrames = 0;
    updateSparkleLight();
}
void TouchReactionMapParts::updateSparkleLight() {
    if (al::isClipped(this) || al::isDead(this) || !mHasSparkle) return;
    if (mLinkedTransUpdated) {
        if (!al::isDead(mLight)) mLight->makeActorDead();
        if (al::isEffectEmitting(this, "Sparkle")) al::tryDeleteEffectAndParticle(this, "Sparkle");
        mLinkedTransUpdated = false;
        return;
    }
    if (mIsDisaster) {
        if (al::isDead(mLight)) mLight->appear();
        if (!al::isEffectEmitting(this, "Sparkle")) al::tryEmitEffect(this, "Sparkle", nullptr);
    }
    if (!mIsDisaster) {
        if (!al::isDead(mLight)) mLight->makeActorDead();
        if (al::isEffectEmitting(this, "Sparkle")) al::tryDeleteEffectAndParticle(this, "Sparkle");
    }
}
void TouchReactionMapParts::onDisasterModeStateChange(DisasterModeController::State state) {
    using State = DisasterModeController::State;
    switch (state) {
    case State::DisasterTransitionStart: case State::Disaster: case State::DisasterTransitionEnd:
    case State::DisasterStart: mIsDisaster = true; break;
    case State::RainStart: return;
    default: mIsDisaster = state == State::DisasterLong; break;
    }
    updateSparkleLight();
}
void TouchReactionMapParts::exeWait() {
    if (!mIsInvisible && al::isFirstStep(this)) al::tryStartAction(this, "Wait");
    updateSparkleLight();
}
void TouchReactionMapParts::exeTouched() {
    if (al::isFirstStep(this)) {
        al::validateHitSensor(this, "Broadcast");
        if (!mIsInvisible) al::tryStartAction(this, "Touched");
    }
    if (mTouchFrames - 1 >= 0) --mTouchFrames;
    if (mTouchFrames == 0) al::startNerveAction(this, "Wait");
}
void TouchReactionMapParts::exeReaction() {
    if (al::isFirstStep(this)) {
        if (!mIsInvisible) al::tryStartAction(this, "Reaction");
        if (mHasSparkle) {
            al::tryDeleteEffect(this, "Sparkle");
            if (!al::isDead(mLight)) mLight->makeActorDead();
        }
    }
    if (al::isGreaterEqualStep(this, 51)) {
        al::startNerveAction(this, "Finish");
        auto* jr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        sead::Vector3f pos;
        if (jr) al::multVecPose(&pos, jr, sItemAppearOffset);
        else pos = al::getSensorPos(al::getHitSensor(this, "Broadcast"));
        al::appearItemTiming(this, "通常アイテム", pos, sead::Vector3f::ey);
        al::invalidateHitSensors(this);
    }
}
void TouchReactionMapParts::exeFinish() {
    if (!mIsInvisible && al::isFirstStep(this)) al::tryStartAction(this, "Finish");
    if (al::isActionEnd(this)) kill();
}
