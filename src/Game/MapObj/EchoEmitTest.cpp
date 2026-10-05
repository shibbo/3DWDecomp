#include "MapObj/EchoEmitTest.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Library/ActorUtil.hpp"

EchoEmitTest::EchoEmitTest(const char* pName) : al::LiveActor(pName) {}

void EchoEmitTest::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    rc::initEchoEmitterHolder(this, rInfo);
    makeActorDead();
}

void EchoEmitTest::exeWait() {}

EchoEmitTest::~EchoEmitTest() {}
