#include "MapObj/LighthouseSimple.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
LighthouseSimple::LighthouseSimple(const char* name) : al::LiveActor(name) {}
void LighthouseSimple::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "Lighthouse", nullptr);
    mInk = new al::LiveActor("LighthouseInk");
    al::initActorWithArchiveName(mInk, info, "LighthouseInk", nullptr);
    al::invalidateHitSensors(mInk);
    makeActorAppeared();
    mInk->makeActorDead();
    al::invalidateCollisionParts(this);
    al::invalidateCollisionParts(mInk);
    mFlag = new al::LiveActor("FlingPoleFlag");
    al::initActorWithArchiveName(mFlag, info, "FlingPoleFlag", nullptr);
    if (al::calcLinkChildNum(info, "FlingPole") == 1) {
        sead::Vector3f trans, rotate;
        al::getChildLinkTR(&trans, &rotate, info, "FlingPole", 0);
        al::resetPosition(mFlag, trans, rotate);
    } else {
        sead::Vector3f rotate;
        al::getRotate(&rotate, info.getPlacementInfo());
        al::resetPosition(mFlag, al::getTrans(this) + sead::Vector3f(0.0f, 2439.1582f, 0.0f), rotate);
    }
    mFlag->makeActorDead();
}
void LighthouseSimple::initAfterPlacement() {
    int island = mPlacementHolder->getZoneNo();
    mIslandId = island;
    setPhaseColor();
    int count = 0;
    if (island - 1 >= 0) {
        const char* names[] = {"GoalItem1_Toggle", "GoalItem2_Toggle", "GoalItem3_Toggle", "GoalItem4_Toggle", "GoalItem5_Toggle"};
        for (int scenario = 0; scenario < 5; ++scenario) {
            if (SingleModeDataFunction::isScenarioComplete(this, island - 1, scenario)) {
                al::startVisAnimAndSetFrameAndStop(this, names[scenario], 1.0f);
                al::startVisAnimAndSetFrameAndStop(mInk, names[scenario], 1.0f);
                ++count;
            }
        }
    }
    if (count == 0) {
        mInk->makeActorAppeared();
        makeActorDead();
        al::startMclAnim(mInk, "LighthouseInkOn");
        al::emitEffect(mInk, "WaitLod01", nullptr);
    } else if (count >= 5) {
        al::emitEffect(this, "CatShineCompleteLod", nullptr);
        if (mFlag) {
            mFlag->makeActorAppeared();
            al::resetPosition(mFlag, al::getTrans(this) + sead::Vector3f(0.0f, 2439.1582f, 0.0f), false);
            al::invalidateHitSensors(mFlag);
            al::startSklAnim(mFlag, "LighthouseFlagFlapDisasterMode");
        }
    } else {
        al::emitEffect(this, "CatShinePartialLod", nullptr);
    }
}
void LighthouseSimple::setPhaseColor() {
    int island = mPlacementHolder->getZoneNo();
    float phase = island < 5 ? 0.0f : island < 9 ? 1.0f : 2.0f;
    if (al::tryStartMclAnimIfExist(this, "PhaseColor")) al::setMclAnimFrameAndStop(this, phase);
    if (al::tryStartVisAnimIfExist(this, "PhaseColor")) al::setVisAnimFrameAndStop(this, phase);
    if (al::tryStartMtpAnimIfExist(this, "PhaseColor")) al::setMtpAnimFrameAndStop(this, phase);
    if (mInk) {
        if (al::tryStartMclAnimIfExist(mInk, "PhaseColor")) al::setMclAnimFrameAndStop(mInk, phase);
        if (al::tryStartVisAnimIfExist(mInk, "PhaseColor")) al::setVisAnimFrameAndStop(mInk, phase);
        if (al::tryStartMtpAnimIfExist(mInk, "PhaseColor")) al::setMtpAnimFrameAndStop(mInk, phase);
    }
}
bool LighthouseSimple::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    return al::isMsgPlayerDisregard(msg);
}
