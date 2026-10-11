#include "MapObj/Fury/FloatingIslandRailPart.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActorFlag.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/RailMoveMovement.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Project/Collision/CollisionParts.hpp"
namespace {
    float lerpSpeed(float rate, float from, float to) { float scaledTo = rate * to; return (1.0f - rate) * from + scaledTo; }
    NERVE_ACTION_IMPL(FloatingIslandRailPart, Move);
    NERVE_ACTION_IMPL(FloatingIslandRailPart, MoveSign);
    NERVE_ACTION_IMPL(FloatingIslandRailPart, StandBy);
    NERVES_MAKE_STRUCT(FloatingIslandRailPart, Move, MoveSign, StandBy)
}
FloatingIslandRailPart::FloatingIslandRailPart(const char* name) : al::RailMoveMapParts(name), mFarLodMtx(sead::Matrix34f::ident) {}
FloatingIslandRailPart::~FloatingIslandRailPart() {}
void FloatingIslandRailPart::init(const al::ActorInitInfo& info) {
    al::RailMoveMapParts::init(info);
    initAnimPart(this, info);
    mAttachedObjects.init(info, false);
    mBaseSpeed = mRailMoveMovement->getSpeed();
    _142 = true;
    mJointMtx = al::getJointMtxPtr(this, "MovingStep_Dsp");
}
void FloatingIslandRailPart::initAfterPlacement() {
    al::makeMtxRT(&mFarLodMtx, this);
    al::LiveActor::initAfterPlacement();
    initAnimPartAfterPlacement(this);
}
void FloatingIslandRailPart::control() {
    const sead::Matrix34f* mtx;
    if (al::isClipped(getFarLodActor())) mtx = mJointMtx;
    else {
        mtx = &mFarLodMtx;
        al::makeMtxRT(&mFarLodMtx, getFarLodActor());
    }
    mAttachedObjects.syncObjectsToPosition(mtx->getTranslation());
    getCollisionParts()->setSyncCollisionMtx(mtx);
    getCollisionParts()->syncMtx();
    getCollisionParts()->updateMtx();
}
void FloatingIslandRailPart::exeMove() {
    al::updateNerveState(this);
    int noTouchFrames = mNoTouchFrames++;
    ++mSpeedChangeFrames;
    if (noTouchFrames >= 300 && !mUnoccupied) {
        mUnoccupied = true;
        mSpeedChangeFrames = 0;
    }
    auto* movement = mRailMoveMovement;
    if (mUnoccupied) {
        if (mSpeedChangeFrames <= 60) {
            float rate = float(mSpeedChangeFrames) / 60.0f;
            movement->setSpeed(lerpSpeed(rate, 7.0f, mBaseSpeed));
        }
    } else if (mSpeedChangeFrames <= 60) {
        float rate = float(mSpeedChangeFrames) / 60.0f;
        movement->setSpeed(rate * 7.0f + (1.0f - rate) * mBaseSpeed);
    }
    if (mUnoccupied) {
        int part = al::getRailPartIndex(this);
        if (part == 1) {
            if (mSpeedChangeFrames > 60) mSpeedChangeFrames = 0;
            movement->setSpeed(al::lerpValue(al::easeIn(float(mSpeedChangeFrames) / 60.0f), mBaseSpeed, 7.0f));
        } else if (part == 2 && mSpeedChangeFrames <= 60) {
            movement->setSpeed(al::lerpValue(al::easeIn(float(mSpeedChangeFrames) / 60.0f), 7.0f, mBaseSpeed));
        }
    }
}
void FloatingIslandRailPart::exeMoveSign() {
    if (al::isFirstStep(this)) {
        mSpeedChangeFrames = 0;
        if (!al::tryStartAction(this, "MoveSign")) { al::startNerveAction(this, "Move"); return; }
    }
    if (al::isActionEnd(this) || !getFarLodActor()->getFlags()->isClipped) {
        if (!mUnoccupied) mSpeedChangeFrames = 60;
        al::startNerveAction(this, "Move");
    }
}
void FloatingIslandRailPart::exeStandBy() {}
bool FloatingIslandRailPart::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgFloorTouch(msg)) {
        if (mUnoccupied) mSpeedChangeFrames = 0;
        mUnoccupied = false;
        mNoTouchFrames = 0;
        return true;
    }
    return al::RailMoveMapParts::receiveMsg(msg, sender, receiver);
}
void FloatingIslandRailPart::onDisasterModeStateChange(DisasterModeController::State state) { startDisasterModeAnim(this, state); }
