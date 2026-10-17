#include "MapObj/NeedleBarRoot.hpp"
#include "MapObj/NeedleBar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/MapObj/SupportFreezeSyncGroupHolder.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
namespace {
    NERVE_DECL(NeedleBarRoot, Wait);
    NERVES_MAKE_NOSTRUCT(NeedleBarRoot, Wait)
}
NeedleBarRoot::NeedleBarRoot(const char* name) : al::LiveActor(name) {}
NeedleBarRoot::~NeedleBarRoot() {}
void NeedleBarRoot::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "NeedleBarRoot", nullptr);
    al::registSupportFreezeSyncGroup(this, info);
    int count = 4;
    al::tryGetArg(&count, info, "BarNum");
    al::tryGetArg(&mRotateSpeed, info, "RotateSpeed");
    auto* group = new al::DeriveActorGroup<NeedleBar>("トゲバーリスト", count);
    mBars = group;
    if (group->mMaxActors >= 1) {
        for (int i = 0; i < group->mMaxActors; ++i) {
            auto* bar = new NeedleBar("トゲバー");
            al::initCreateActorWithPlacementInfo(bar, info);
            group->registerActor(bar);
        }
    }
    setBarRotate(true);
    al::setClippingInfo(this, al::getClippingRadius(mBars->getDeriveActor(0)), nullptr);
    al::initNerve(this, &NrvNeedleBarRootWait, 0);
    makeActorAppeared();
}
void NeedleBarRoot::setBarRotate(bool reset) {
    int count = mBars->mNumActors;
    for (int i = 0; i < count; ++i) {
        mBars->getDeriveActor(i)->setRotateY(mAngle + (float(i) / float(count)) * 360.0f, reset);
    }
}
bool NeedleBarRoot::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgIsNerveSupportFreeze(msg)) {
        for (int i = 0; i < mBars->mNumActors; ++i)
            if (mBars->getDeriveActor(i)->isNerveSupportFreeze()) return true;
        return false;
    }
    if (al::isMsgOnSyncSupportFreeze(msg)) {
        for (int i = 0; i < mBars->mNumActors; ++i) mBars->getDeriveActor(i)->onSyncSupportFreeze();
        return true;
    }
    if (al::isMsgOffSyncSupportFreeze(msg)) {
        for (int i = 0; i < mBars->mNumActors; ++i) mBars->getDeriveActor(i)->offSyncSupportFreeze();
        return true;
    }
    return false;
}
void NeedleBarRoot::exeWait() {
    for (int i = 0; i < mBars->mNumActors; ++i)
        if (mBars->getDeriveActor(i)->isStop()) return;
    mAngle = al::wrapAngle(mAngle + mRotateSpeed);
    setBarRotate(false);
    al::holdSe(this, "PgRotateLv", nullptr);
}
