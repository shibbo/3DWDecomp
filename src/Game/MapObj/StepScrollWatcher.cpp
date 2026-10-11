#include "MapObj/StepScrollWatcher.hpp"
#include "MapObj/StepScrollParts.hpp"
#include "Library/MapObj/FixMapParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
namespace {
    NERVE_DECL(StepScrollWatcher, Wait);
    NERVE_DECL(StepScrollWatcher, Accel);
    NERVES_MAKE_NOSTRUCT(StepScrollWatcher, Wait, Accel)
}
StepScrollWatcher::StepScrollWatcher(const char* name) : al::LiveActor(name) {}
StepScrollWatcher::~StepScrollWatcher() = default;
void StepScrollWatcher::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorSRT(this, info);
    al::initExecutorWatchObj(this, info);
    al::initActorClipping(this, info);
    al::initStageSwitch(this, info);
    al::onDrawClipping(this);
    al::tryGetArg(&mLoopStepWidth, info, "LoopStepWidth");
    al::tryGetArg(&mLoopDistance, info, "LoopDistance");
    float startPoint;
    int axis;
    al::tryGetArg(&axis, info, "MoveAxis");
    al::tryGetArg(&mInitialSpeed, info, "ScrollInitSpeed");
    al::tryGetArg(&mMaxSpeed, info, "ScrollMaxSpeed");
    al::tryGetArg(&startPoint, info, "ScrollStartPoint");
    al::tryGetArg(&mAccelTiming, info, "AccelTimingFrame");
    al::tryGetArg(&mUseCameraShake, info, "IsUseCameraShake");
    mSpeed = mInitialSpeed;
    switch (axis) {
    case 0: mDirection.set(1.0f, 0.0f, 0.0f); break;
    case 1: mDirection.set(0.0f, 1.0f, 0.0f); break;
    case 2: mDirection.set(0.0f, 0.0f, 1.0f); break;
    }
    mOneTimeCount = al::calcLinkChildNum(info, "OneTimeScrollStepLink");
    mLoopCount = al::calcLinkChildNum(info, "LoopScrollStepLink");
    mWheelCount = al::calcLinkChildNum(info, "WheelStepLink");
    mOneTimeParts = new StepScrollParts*[mOneTimeCount];
    for (int i = 0; i < mOneTimeCount; ++i) {
        mOneTimeParts[i] = new StepScrollParts("ワンタイムスクロール足場");
        al::initLinksActor(mOneTimeParts[i], info, "OneTimeScrollStepLink", i);
        al::setVelocity(mOneTimeParts[i], sead::Vector3f(0.0f, 0.0f, 0.0f));
        al::onDrawClipping(mOneTimeParts[i]);
    }
    mLoopCount = mOneTimeCount + int(mLoopDistance / mLoopStepWidth);
    mLoopParts = new StepScrollParts*[mLoopCount];
    for (int i = 0; i < mLoopCount; ++i) {
        mLoopParts[i] = new StepScrollParts("ループスクロール足場");
        al::initLinksActor(mLoopParts[i], info, "LoopScrollStepLink", 0);
        const sead::Vector3f& trans = al::getTrans(mLoopParts[i]);
        sead::Vector3f pos = (mLoopStepWidth * mDirection) * float(i) + trans;
        al::resetPosition(mLoopParts[i], pos, false);
        al::setVelocity(mLoopParts[i], sead::Vector3f(0.0f, 0.0f, 0.0f));
        al::onDrawClipping(mLoopParts[i]);
        al::setIgnoreUpdateDrawClipping(mLoopParts[i], true);
    }
    mWheels = new al::FixMapParts*[mWheelCount];
    mWheelCircumferences = new float[mWheelCount];
    mWheelFrames = new float[mWheelCount];
    for (int i = 0; i < mWheelCount; ++i) {
        mWheels[i] = new al::FixMapParts("車輪足場");
        al::initLinksActor(mWheels[i], info, "WheelStepLink", i);
        al::setActionFrameRate(mWheels[i], 0.0f);
        mWheelCircumferences[i] = 0.0f;
        mWheelFrames[i] = 0.0f;
    }
    mBoundary = startPoint * mDirection;
    mLoopIndex = 0;
    for (int i = 0; i < mWheelCount; ++i) {
        float radius;
        auto* wheel = mWheels[i];
        if (al::isExistModelResourceYaml(wheel, "Wheel", nullptr)) {
            al::ByamlIter iter(al::getModelResourceYaml(wheel, "Wheel", nullptr));
            al::tryGetByamlF32(&radius, iter, "Radius");
        }
        const char* action = al::getActionName(mWheels[i]);
        mWheelFrames[i] = al::getActionFrameMax(mWheels[i], action);
        mWheelCircumferences[i] = radius * 6.283185307179586;
    }
    al::listenStageSwitchOnKill(this, al::FunctorV0M(this, &StepScrollWatcher::kill));
    al::initNerve(this, &NrvStepScrollWatcherWait, 0);
    makeActorAppeared();
}
void StepScrollWatcher::control() {
    if (al::isNerve(this, &NrvStepScrollWatcherAccel))
        mSpeed = al::calcNerveSquareOutValue(this, 1000, mInitialSpeed, mMaxSpeed);
    for (int i = 0; i < mWheelCount; ++i) {
        float rate = 0.0f;
        if (mSpeed > 0.0f) rate = mWheelFrames[i] / (mWheelCircumferences[i] / mSpeed);
        al::setActionFrameRate(mWheels[i], rate);
    }
    for (int i = 0; i < mOneTimeCount; ++i) {
        if (!al::isDead(mOneTimeParts[i])) mOneTimeParts[i]->scroll(mDirection, mSpeed);
    }
    for (int i = 0; i < mLoopCount; ++i) mLoopParts[i]->scroll(mDirection, mSpeed);
    for (int i = 0; i < mOneTimeCount; ++i) {
        if (!al::isDead(mOneTimeParts[i])) {
            if (!mOneTimeParts[i]->isOverBound(mDirection, mBoundary)) return;
            mOneTimeParts[i]->kill();
        }
    }
    if (!mLoopParts[mLoopIndex]->isOverBound(mDirection, mBoundary)) return;
    int previous = mLoopIndex;
    if (previous == 0) previous = mLoopCount;
    --previous;
    const sead::Vector3f& trans = al::getTrans(mLoopParts[previous]);
    sead::Vector3f pos = mLoopStepWidth * mDirection + trans;
    al::resetPosition(mLoopParts[mLoopIndex], pos, false);
    ++mLoopIndex;
    if (mLoopIndex == mLoopCount) mLoopIndex = 0;
}
void StepScrollWatcher::kill() {
    for (int i = 0; i < mOneTimeCount; ++i) mOneTimeParts[i]->kill();
    for (int i = 0; i < mLoopCount; ++i) mLoopParts[i]->kill();
    for (int i = 0; i < mWheelCount; ++i) mWheels[i]->kill();
    al::LiveActor::kill();
}
void StepScrollWatcher::exeWait() {
    if (al::isGreaterEqualStep(this, mAccelTiming)) al::setNerve(this, &NrvStepScrollWatcherAccel);
}
void StepScrollWatcher::exeAccel() {
    if (mUseCameraShake) {
        if (++mShakeStep > 200) {
            mShakeStep = 0;
            al::requestStartCameraShake(this, "微弱");
        }
    }
}
