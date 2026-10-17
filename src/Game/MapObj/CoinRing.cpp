#include "MapObj/CoinRing.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
namespace {
NERVE_DECL(CoinRing, Stop);
NERVE_DECL(CoinRing, Delay);
NERVE_DECL(CoinRing, Move);
NERVE_DECL(CoinRing, Disappear);
NERVE_DECL(CoinRing, Wait);
NERVES_MAKE_NOSTRUCT(CoinRing, Disappear)
NERVES_MAKE_STRUCT(CoinRing, Stop, Delay, Move, Wait)
}
CoinRing::CoinRing(const char* name) : al::LiveActor(name) {}
CoinRing::~CoinRing() {}
void CoinRing::init(const al::ActorInitInfo& info) {
    int shadowType = 0;
    bool hasType = al::tryGetArg(&shadowType, info, "ShadowPlacementType");
    const char* suffix = hasType && shadowType == 1 ? "SidePlacement" : nullptr;
    al::initActorWithArchiveName(this, info, "CoinRing", suffix);
    al::initNerve(this, &NrvCoinRing.Stop, 0);
    al::startAction(this, "Wait");
    mHasKeyMove = al::calcLinkChildNum(info, "KeyMoveNext") > 0;
    if (mHasKeyMove) {
        mKeyPose = al::createKeyPoseKeeper(info);
        al::tryGetArg(&mDelayTime, info, "DelayTime");
        if (al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &CoinRing::startKeyMove))) al::setNerve(this, &NrvCoinRing.Stop);
        else startKeyMove();
    }
    bool appear = true;
    if (al::listenStageSwitchOnOff(this, "SwitchTimerAppear", al::FunctorV0M(this, &CoinRing::show), al::FunctorV0M(this, &CoinRing::hide))) {
        al::setNerve(this, &NrvCoinRing.Stop);
        appear = false;
    }
    al::calcShadowMaskSize(&mShadowSize, this, "シャドウマスク");
    mShadowScale = al::getShadowTextureFixedScale(this, "シャドウマスク");
    al::trySetShadowLength(this, info, nullptr);
    mSingleMode = info.mActorSceneInfo.isSingleMode;
    if (mSingleMode) mPlacementIndex = info.mPlacementInfo->_28;
    if (appear) makeActorAppeared();
    else makeActorDead();
}
void CoinRing::startKeyMove() {
    if (mDelayTime > 0) al::setNerve(this, &NrvCoinRing.Delay);
    else al::setNerve(this, &NrvCoinRing.Move);
}
void CoinRing::show() { makeActorAppeared(); reappear(); }
void CoinRing::hide() { makeActorDead(); }
void CoinRing::initAfterPlacement() {
    al::updateMaterialCodeWater(this);
    if (mSingleMode) {
        auto* watcher = al::getSceneObj<CloudBonusWatcher>(this, 52);
        if (watcher) watcher->tryRegisterActor(this, mPlacementIndex);
    }
}
bool CoinRing::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvCoinRingDisappear)) return false;
    if (al::isMsgItemGetAll(msg)) {
        if (al::isSensorHitRingShape(sender, receiver, 40.0f)) {
            al::setAppearItemAttackerSensor(this, sender);
            al::setNerve(this, &NrvCoinRingDisappear);
            return true;
        }
    }
    return false;
}
void CoinRing::reappear() {
    al::startAction(this, "Wait");
    if (mSingleMode) {
        al::showShadow(this);
        al::setShadowMaskSize(this, "シャドウマスク", mShadowSize);
        al::setShadowTextureFixedScale(this, "シャドウマスク", mShadowScale);
    }
    if (!mHasKeyMove || al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &CoinRing::startKeyMove))) al::setNerve(this, &NrvCoinRing.Stop);
    else startKeyMove();
    makeActorAppeared();
}
void CoinRing::exeWait() {
    if (al::isFirstStep(this)) {
        mWaitTime = al::calcKeyMoveWaitTime(mKeyPose);
        if (mWaitTime < 0) { al::setNerve(this, &NrvCoinRing.Move); return; }
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) al::setNerve(this, &NrvCoinRing.Move);
}
void CoinRing::exeMove() {
    if (al::isFirstStep(this)) mMoveTime = al::calcKeyMoveMoveTime(mKeyPose);
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPose, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPose, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPose);
        al::setNerve(this, al::isStop(mKeyPose) ? static_cast<const al::Nerve*>(&NrvCoinRing.Stop) : &NrvCoinRing.Wait);
    }
}
void CoinRing::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayTime)) al::setNerve(this, &NrvCoinRing.Move);
}
void CoinRing::exeStop() {}
void CoinRing::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Disappear");
        al::killPrePassLightAll(this, -1);
    }
    if (!al::isActionEnd(this)) {
        float rate = al::normalize(al::getActionFrame(this), 0.0f, al::getActionFrameMax(this, "Disappear"));
        sead::Vector3f size(0.0f, 0.0f, 0.0f);
        sead::Vector3f end(0.0f, mShadowSize.y, 0.0f);
        al::lerpVec(&size, mShadowSize, end, rate);
        al::setShadowMaskSize(this, "シャドウマスク", size);
        al::setShadowTextureFixedScale(this, "シャドウマスク", al::lerpValue(rate, mShadowScale, 0.001f));
    } else if (al::isActionEnd(this) && !al::isHideShadow(this)) al::hideShadow(this);
    if (al::isIntervalStep(this, 8, 0)) {
        int index = 0;
        if (al::getNerveStep(this) != 0) {
            index = al::getNerveStep(this) / 8;
            if (index >= 3) { kill(); return; }
        }
        sead::Vector3f front;
        sead::Vector3f up(0.0f, 0.0f, 0.0f);
        front.set(0.0f, 0.0f, 0.0f);
        al::calcUpDir(&up, this);
        al::calcFrontDir(&front, this);
        if (!al::isParallelDirection(up, front, 0.01f)) {
            al::rotateVectorDegree(&up, up, front, float(index) * 120.0f);
            al::normalize(&up);
            up *= 100.0f;
            up.y += 30.0f;
        }
        al::setAppearItemOffset(this, up);
        al::appearItem(this);
    }
}
