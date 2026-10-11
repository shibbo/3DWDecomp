#include "MapObj/LanternRing.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(LanternRing, Wait);
    NERVES_MAKE_NOSTRUCT(LanternRing, Wait)
}

LanternRing::LanternRing(const char* pName) : al::LiveActor(pName) {}

LanternRing::~LanternRing() {}

void LanternRing::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, "LanternRing", nullptr);
    al::initNerve(this, &NrvLanternRingWait, 0);
    makeActorAppeared();
}

void LanternRing::setPrevTrans() {
    mPrevTrans.set(al::getTrans(this));
}

void LanternRing::setPrevQuat() {
    mPrevQuat.set(al::getQuat(this));
}

void LanternRing::exeWait() {}

void LanternRing::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther) && al::isSensorName(pSelf, "Push"))
        al::sendMsgPush(pOther, pSelf);
}
