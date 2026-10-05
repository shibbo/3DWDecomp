#include "MapObj/SwitchRotateWatcher.hpp"
#include "MapObj/SwitchRotateParts.hpp"
#include "MapObj/SwitchRotateSwitch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
    NERVE_DECL(SwitchRotateWatcher, Wait);
    NERVE_DECL(SwitchRotateWatcher, Rotate);
    NERVES_MAKE_NOSTRUCT(SwitchRotateWatcher, Wait, Rotate)
}

SwitchRotateWatcher::SwitchRotateWatcher(const char* pName) : al::LiveActor(pName) {
}

void SwitchRotateWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::tryGetArg(&mClockAngle, rInfo, "ClockAngle");
    al::tryGetArg(&mRotateTime, rInfo, "RotateTime");
    al::tryGetArg(&mRotateAxis, rInfo, "RotateAxis");
    mInitialQuat.set(al::getQuat(this));
    al::makeMtxSRT(&mMtx, this);
    sead::Matrix34f inverseMtx;
    inverseMtx.setInverse(mMtx);
    al::initNerve(this, &NrvSwitchRotateWatcherWait, 0);
    mPartsCount = al::calcLinkChildNum(rInfo, "RotateObjectLink");
    mSwitchCount = al::calcLinkChildNum(rInfo, "RotateSwitchLink");
    al::initSubActorKeeperNoFile(this, rInfo, mPartsCount + mSwitchCount);
    mParts = new SwitchRotateParts*[mPartsCount];
    for (int i = 0; i < mPartsCount; ++i) {
        mParts[i] = new SwitchRotateParts("回転パーツ");
        al::initLinksActor(mParts[i], rInfo, "RotateObjectLink", i);
        mParts[i]->initFromWatcher(&mMtx, inverseMtx);
        al::registerSubActorSyncClipping(this, mParts[i], false);
    }
    mSwitches = new SwitchRotateSwitch*[mSwitchCount];
    for (int i = 0; i < mSwitchCount; ++i) {
        mSwitches[i] = new SwitchRotateSwitch("回転スイッチ");
        al::initLinksActor(mSwitches[i], rInfo, "RotateSwitchLink", i);
        al::registerSubActorSyncClipping(this, mSwitches[i], false);
    }
    calcClippingRange();
    makeActorAppeared();
}

void SwitchRotateWatcher::calcClippingRange() {
    const sead::Vector3f& trans = al::getTrans(this);
    float radius = 1.0f;
    for (int i = 0; i < mPartsCount; ++i) {
        float distance = (trans - al::getTrans(mParts[i])).length();
        float partRadius = distance + al::getClippingRadius(mParts[i]);
        if (radius < partRadius) {
            radius = partRadius;
        }
    }
    for (int i = 0; i < mSwitchCount; ++i) {
        float distance = (trans - al::getTrans(mSwitches[i])).length();
        float switchRadius = distance + al::getClippingRadius(mSwitches[i]);
        if (radius < switchRadius) {
            radius = switchRadius;
        }
    }
    al::setClippingInfo(this, radius, &trans);
}

void SwitchRotateWatcher::exeWait() {
    if (al::isLessEqualStep(this, 5)) {
        return;
    }
    for (int i = 0; i < mSwitchCount; ++i) {
        if (mSwitches[i]->isOnSwitch()) {
            al::setNerve(this, &NrvSwitchRotateWatcherRotate);
            return;
        }
    }
}

void SwitchRotateWatcher::exeRotate() {
    if (al::isFirstStep(this)) {
        for (int i = 0; i < mSwitchCount; ++i) {
            mSwitches[i]->requestOffSwitch(mRotateTime);
        }
        for (int i = 0; i < mPartsCount; ++i) {
            mParts[i]->requestRotate();
            al::tryStartSe(mParts[i], "RotateStart", nullptr);
        }
    }
    float angle = (mRotateCount + al::calcNerveRate(this, mRotateTime)) * mClockAngle;
    angle = al::wrapValue(angle, 360.0f);
    al::rotateQuatLocalDirDegree(this, mInitialQuat, mRotateAxis, angle);
    al::makeMtxSRT(&mMtx, this);
    if (al::isGreaterStep(this, mRotateTime)) {
        ++mRotateCount;
        for (int i = 0; i < mPartsCount; ++i) {
            mParts[i]->requestStop();
        }
        al::setNerve(this, &NrvSwitchRotateWatcherWait);
    }
}

SwitchRotateWatcher::~SwitchRotateWatcher() {
}
