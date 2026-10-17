#include "MapObj/Trampoline.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(Trampoline, Wait)
NERVE_DECL(Trampoline, Stop)
NERVE_DECL(Trampoline, Ready)
NERVE_DECL(Trampoline, Move)
NERVES_MAKE_NOSTRUCT(Trampoline, Move)
NERVES_MAKE_STRUCT(Trampoline, Wait, Stop, Ready)
const PlayerBindEndParam sTrampolineBindEndParam = {"Jump", 1, 10, true, true, true, 0, 1.2f, 0, false, {}, 0.0f};
}  // namespace

Trampoline::Trampoline(const char* name) : al::LiveActor(name) {}
void Trampoline::init(const al::ActorInitInfo& info) {
    const char* suffix = nullptr;
    if (al::isSingleMode(info)) {
        const char* model;
        alPlacementFunction::tryGetModelName(&model, info);
        if (al::isEqualString(model, "Trampoline8x8M")) suffix = "SM";
    }
    al::initMapPartsActor(this, info, suffix, 0);
    al::initNerve(this, &NrvTrampoline.Wait, 0);
    bool hideShadow = false;
    al::tryGetArg(&mJumpPower, info, "JumpPower");
    al::tryGetArg(&mHighJumpPower, info, "HighJumpPower");
    al::tryGetArg(&mFloorTouchStart, info, "IsFloorTouchStart");
    al::tryGetArg(&mKeepVelocity, info, "IsKeepPlayerVelocity");
    al::tryGetArg(&hideShadow, info, "IsHideShadowMask");
    mKeyPose = al::createKeyPoseKeeper(info);
    if (al::getKeyPoseCount(mKeyPose) == 1) {
        al::setNerve(this, &NrvTrampoline.Stop);
        mConnector = al::tryCreateMtxConnector(this, info);
    } else {
        float radius;
        al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPose, 0.0f);
        al::setClippingInfo(this, radius + al::getClippingRadius(this), &mClippingCenter);
        if (mFloorTouchStart) al::setNerve(this, &NrvTrampoline.Ready);
    }
    if (al::isExistAction(this, "Through")) al::setEffectFollowPosPtr(this, "Through", &mThroughPos);
    makeActorAppeared();
    if (hideShadow) al::invalidateShadow(this, static_cast<al::ShadowMaskDrawCategory>(2));
    al::startAction(this, "Wait");
    mRumble = al::createPadRumbleKeeper(this, al::getMainControllerPort());
}
void Trampoline::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
    al::updateMaterialCodeWater(this);
}
void Trampoline::control() {
    if (mLastBoundTimer - 1 >= 0) {
        --mLastBoundTimer;
        if (mLastBoundTimer == 0) mLastBound = nullptr;
    }
    if (mBoundTimer - 1 >= 0) --mBoundTimer;
    tryJumpPlayer();
    if (mConnector) al::connectPoseQT(this, mConnector);
    if (!al::isActionPlaying(this, "Wait") && al::isActionEnd(this)) al::startAction(this, "Wait");
}
void Trampoline::tryJumpPlayer() {
    if (mPlayers.size() >= 1) {
        bool high = false;
        for (int i = 0; i < mPlayers.size(); ++i) {
            IUsePlayerPuppet* puppet = rc::startPuppet(al::getHitSensor(this, "Bind"), mPlayers[i]);
            sead::Vector3f up;
            al::calcUpDir(&up, this);
            sead::Vector3f velocity = sead::Vector3f::zero;
            if (mKeepVelocity) al::verticalizeVec(&velocity, up, rc::getPuppetVelocity(puppet));
            int port = rc::getPlayerInputPort(rc::getPuppetSensor(puppet));
            mRumble->setPort(port);
            if (rc::isPuppetAction(puppet, "HipDrop") || rc::isPuppetAction(puppet, "GiantHipDrop") ||
                rc::isPuppetAction(puppet, "WaitStatue") || static_cast<unsigned>(rc::getPuppetTrigJumpFrame(puppet)) <= 4) {
                up *= mHighJumpPower;
                rc::startPuppetSe(puppet, "JumpHigh");
                al::startHitReaction(this, "ジャンプ大");
                high = true;
            } else {
                up *= mJumpPower;
                rc::startPuppetSe(puppet, "JumpVoice");
                al::startHitReaction(this, "ジャンプ");
            }
            velocity += up;
            sead::Vector3f direction;
            if (!al::normalizeOrZero(&direction, velocity)) {
                direction.x += (sead::Vector3f::ey.x - direction.x) * 0.5f;
                direction.y += (sead::Vector3f::ey.y - direction.y) * 0.5f;
                direction.z += (sead::Vector3f::ey.z - direction.z) * 0.5f;
                velocity = velocity.length() * direction;
            }
            rc::setPuppetVelocity(puppet, velocity);
            rc::endBindAndPuppetNull(&puppet, &sTrampolineBindEndParam);
        }
        al::startAction(this, high ? "ReactionHigh" : "Reaction");
        mReacting = true;
        mPlayers.clear();
        mBoundTimer = 3;
    }
    if (mReacting && al::isActionEnd(this)) mReacting = false;
}
void Trampoline::attackSensor(al::HitSensor*, al::HitSensor* receiver) {
    if (al::isActionPlaying(this, "Reaction") || al::isActionPlaying(this, "ReactionHigh") || !al::isSensorPlayer(receiver)) return;
    al::LiveActor* player = al::getSensorHost(receiver);
    if (al::getTrans(this).y - 100.0f >= al::getTrans(player).y) return;
    if (al::getTrans(player).y >= al::getTrans(this).y - 10.0f) return;
    if (al::getVelocity(player).y <= 5.0f || al::isActionPlaying(this, "Through")) return;
    mThroughPos.set(al::getTrans(player));
    al::startAction(this, "Through");
}
bool Trampoline::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerFloorTouch(msg)) {
        rc::requestPlayerBind(sender, al::getHitSensor(this, "Bind"));
        return true;
    }
    if (rc::isMsgEnemyFloorTouchTrampoline(msg)) return mBoundTimer > 0;
    if (al::isMsgPlessieFloorTouch(msg)) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidon->forceJump() && mFloorTouchStart && al::isNerve(this, &NrvTrampoline.Ready))
            al::setNerve(this, &NrvTrampoline.Wait);
    }
    if (al::isSensorKoopaJr(sender) && al::isMsgPlayerHipDropAll(msg)) {
        if (rc::sendMsgBoundTrampoline(sender, receiver)) {
            al::setVelocityY(al::getSensorHost(sender), mHighJumpPower);
            al::startHitReaction(this, "ジャンプ大");
            al::startAction(this, "ReactionHigh");
            mReacting = true;
            mBoundTimer = 3;
        }
        return true;
    }
    if (rc::isMsgBoundTrampoline(msg) || (al::isSingleMode(this) && al::isMsgExplosion(msg))) {
        if (!al::isActionPlaying(this, "Reaction") && !al::isActionPlaying(this, "ReactionHigh") && mLastBoundTimer == 0)
            al::startAction(this, "Reaction");
        if (al::isSingleMode(this) && mLastBound != sender) {
            mLastBound = sender;
            if (al::isMsgExplosion(msg)) mLastBoundTimer = 61;
            else mLastBoundTimer = 0;
        }
        return true;
    }
    if (al::isMsgBindStart(msg)) return !(rc::getPlayerVelocity(sender).y > 0.0f);
    if (al::isMsgBindInit(msg)) {
        mPlayers.pushBack(sender);
        if (mFloorTouchStart && al::isNerve(this, &NrvTrampoline.Ready)) al::setNerve(this, &NrvTrampoline.Wait);
        return true;
    }
    if (al::isMsgBindCancel(msg)) {
        mPlayers.erase(mPlayers.indexOf(sender));
        return true;
    }
    return false;
}
void Trampoline::exeReady() {}
void Trampoline::exeWait() {
    if (al::isFirstStep(this)) mMoveTime = al::calcKeyMoveWaitTime(mKeyPose);
    if (al::isGreaterEqualStep(this, mMoveTime)) al::setNerve(this, &NrvTrampolineMove);
}
void Trampoline::exeMove() {
    if (al::isFirstStep(this)) {
        mMoveTime = al::calcKeyMoveMoveTime(mKeyPose);
        if (mFloorTouchStart && mFirstMove) {
            mFirstMove = false;
            al::startSe(this, "pgReady", nullptr);
        }
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPose, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPose);
        if (al::isStop(mKeyPose)) al::setNerve(this, &NrvTrampoline.Stop);
        else al::setNerve(this, &NrvTrampoline.Wait);
    }
    al::holdSe(this, "pgMove", nullptr);
}
void Trampoline::exeStop() {}
