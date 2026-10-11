#include "MapObj/SePlayObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ActorPoseKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
    NERVE_DECL(SePlayObj, Wait);
    NERVE_DECL(SePlayObj, Attached);
    NERVES_MAKE_NOSTRUCT(SePlayObj, Wait, Attached)
}

SePlayObj::SePlayObj(const char* pName) : al::LiveActor(pName) {}
SePlayObj::~SePlayObj() {}

void SePlayObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRMSV(this);
    al::initActorSRT(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorAudioKeeper(this, rInfo, mAudioKeeperName, nullptr);
    al::initActorClipping(this, rInfo);
    if (!mIsAttached) {
        al::tryGetStringArg(&mSeName, rInfo, "SePlayName");
        if (!al::tryGetArg(&mIsStartSeBySwitch, rInfo, "IsStartSeBySwitch"))
            mIsStartSeBySwitch = false;
        if (!al::tryGetArg(&mIsValidClipping, rInfo, "IsValidClipping"))
            mIsValidClipping = true;
        al::tryGetArg(&mClippingRadius, rInfo, "ClippingRadius");
    }
    if (mIsValidClipping) {
        al::validateClipping(this);
        al::setClippingInfo(this, mClippingRadius, nullptr);
        al::setClippingNearFarDistance(this, mClippingRadius, mClippingRadius);
    } else {
        al::invalidateClipping(this);
    }
    if (rInfo.getActorSceneInfo().isSingleMode)
        al::listenStageSwitchOff(this, "SwitchTrampleOn", al::Functor(this, &SePlayObj::switchTrampleOff));
    if (mIsAttached)
        al::initNerve(this, &NrvSePlayObjAttached, 0);
    else
        al::initNerve(this, &NrvSePlayObjWait, 0);
    makeActorAppeared();
}

void SePlayObj::switchTrampleOff() { mSwitchHandled = false; }

void SePlayObj::initWithAudioKeeper(const al::ActorInitInfo& rInfo, const char* pAudioKeeperName) {
    mIsAttached = true;
    mUpdatePose = true;
    mAudioKeeperName = pAudioKeeperName;
    init(rInfo);
}

void SePlayObj::exeWait() {
    if (al::isFirstStep(this) && !mIsStartSeBySwitch)
        al::startSe(this, mSeName, nullptr);
    if (!mSwitchHandled && al::isOnStageSwitch(this, "SwitchTrampleOn")) {
        if (mIsStartSeBySwitch)
            al::startSe(this, mSeName, nullptr);
        else
            al::stopSe(this, mSeName, 0);
        mSwitchHandled = true;
    }
}

void SePlayObj::exeAttached() {
    if (mUpdatePose) {
        const sead::Vector3f& velocity = getPoseKeeper()->getVelocity();
        getPoseKeeper()->mTranslation += velocity;
        alActorPoseFunction::updatePoseTRMSV(this);
    }
}
