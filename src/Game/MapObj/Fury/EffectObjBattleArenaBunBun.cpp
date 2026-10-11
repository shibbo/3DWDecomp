#include "MapObj/Fury/EffectObjBattleArenaBunBun.hpp"
#include "Layout/IslandMap.hpp"
#include "Util/DemoUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/EffectObjFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
    NERVE_DECL(EffectObjBattleArenaBunBun, Appeared);
    NERVE_DECL(EffectObjBattleArenaBunBun, Wait);
    NERVES_MAKE_NOSTRUCT(EffectObjBattleArenaBunBun, Appeared, Wait)
}

EffectObjBattleArenaBunBun::EffectObjBattleArenaBunBun(const char* pName)
    : al::EffectObj(pName) {}
EffectObjBattleArenaBunBun::~EffectObjBattleArenaBunBun() {}

void EffectObjBattleArenaBunBun::init(const al::ActorInitInfo& rInfo) {
    al::EffectObjFunction::initActorEffectObj(this, rInfo);
    al::initNerve(this, &NrvEffectObjBattleArenaBunBunAppeared, 0);
    al::makeMtxRT(&mBaseMtx, this);
    al::trySyncStageSwitchAppear(this);
    al::tryListenStageSwitchKill(this);
    al::listenStageSwitchOnOff(this, "OnKillOffAppearSwitch",
        al::Functor(this, &EffectObjBattleArenaBunBun::kill),
        al::Functor(this, &EffectObjBattleArenaBunBun::appear));
    mMtxConnector = al::tryCreateMtxConnector(this, rInfo);
}

void EffectObjBattleArenaBunBun::appear() {
    al::EffectObj::appear();
    IslandMap::setIslandWarpEnable(this, false);
    rc::addDemoActor(this);
    al::setNerve(this, &NrvEffectObjBattleArenaBunBunAppeared);
}

void EffectObjBattleArenaBunBun::exeAppeared() {
    if (al::isStep(this, 2)) {
        al::startSe(this, "Appear", nullptr);
        rc::removeDemoActor(this);
        al::setNerve(this, &NrvEffectObjBattleArenaBunBunWait);
    }
}

void EffectObjBattleArenaBunBun::exeWait() {}

void EffectObjBattleArenaBunBun::kill() {
    al::EffectObj::kill();
    IslandMap::setIslandWarpEnable(this, true);
}
