#include "MapObj/TestKitazonoBlockBrickBreak.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(TestKitazonoBlockBrickBreak, Wait);
    NERVE_DECL(TestKitazonoBlockBrickBreak, Break);
    NERVES_MAKE_NOSTRUCT(TestKitazonoBlockBrickBreak, Wait, Break)
}

TestKitazonoBlockBrickBreak::TestKitazonoBlockBrickBreak(const char* pName) : al::LiveActor(pName) {}
TestKitazonoBlockBrickBreak::~TestKitazonoBlockBrickBreak() {}

void TestKitazonoBlockBrickBreak::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BlockBrickBreak", nullptr);
    al::initNerve(this, &NrvTestKitazonoBlockBrickBreakWait, 0);
    makeActorDead();
}

void TestKitazonoBlockBrickBreak::control() {}

void TestKitazonoBlockBrickBreak::breakBlock(const sead::Vector3f& rTrans) {
    al::setTrans(this, rTrans);
    makeActorAppeared();
    al::setNerve(this, &NrvTestKitazonoBlockBrickBreakBreak);
}

void TestKitazonoBlockBrickBreak::exeWait() {}

void TestKitazonoBlockBrickBreak::exeBreak() {
    if (al::isFirstStep(this))
        al::startAction(this, "Break");
    if (al::isActionEnd(this))
        kill();
}
