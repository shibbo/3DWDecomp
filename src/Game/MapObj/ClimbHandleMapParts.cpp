#include "MapObj/ClimbHandleMapParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
    NERVE_ACTION_IMPL(ClimbHandleMapParts, Wait)
    NERVE_ACTION_IMPL(ClimbHandleMapParts, Stop)
    NERVE_ACTION_IMPL(ClimbHandleMapParts, Up)
    NERVE_ACTION_IMPL(ClimbHandleMapParts, Down)
    NERVE_ACTION_IMPL_(ClimbHandleMapParts, Move, Wait)
    NERVE_ACTIONS_MAKE_STRUCT(ClimbHandleMapParts, Wait, Stop, Up, Down, Move)
}

ClimbHandleMapParts::ClimbHandleMapParts(const char* pName) : al::LiveActor(pName) {}
ClimbHandleMapParts::~ClimbHandleMapParts() {}

void ClimbHandleMapParts::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    al::initNerveAction(this, "Wait", &NrvClimbHandleMapParts.collector, 0);
    al::initActorPoseTQSV(this);
    const char* archive = mIsSingleMode ? "ClimbHandleMapPartsSM" : "ClimbHandleMapParts";
    if (!al::tryGetMapPartsSuffix(rInfo, archive))
        archive = "ClimbHandleMapParts";
    al::initMapPartsActor(this, rInfo, al::tryGetMapPartsSuffix(rInfo, archive), 0);
    if (getEffectKeeper()) {
        al::makeMtxSRT(&mEffectMtx, this);
        al::setEffectFollowMtxPtr(this, "Move", &mEffectMtx);
    }
    makeActorAppeared();
}

void ClimbHandleMapParts::exeWait() {}
void ClimbHandleMapParts::exeStop() {}
void ClimbHandleMapParts::exeUp() {}
void ClimbHandleMapParts::exeDown() {}
void ClimbHandleMapParts::setNerveToStop() { al::startNerveAction(this, "Stop"); }
void ClimbHandleMapParts::setNerveToUp() { al::startNerveAction(this, "Up"); }
void ClimbHandleMapParts::setNerveToDown() { al::startNerveAction(this, "Down"); }
