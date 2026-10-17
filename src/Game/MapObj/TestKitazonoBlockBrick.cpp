#include "MapObj/TestKitazonoBlockBrick.hpp"
#include "MapObj/TestKitazonoBlockBrickBreak.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
namespace {
NERVE_DECL(TestKitazonoBlockBrick, Move);
NERVE_DECL(TestKitazonoBlockBrick, Wait);
NERVE_DECL(TestKitazonoBlockBrick, Stop);
NERVE_DECL(TestKitazonoBlockBrick, Reaction);
NERVE_DECL(TestKitazonoBlockBrick, Break);
NERVES_MAKE_STRUCT(TestKitazonoBlockBrick, Move, Wait, Reaction, Break)
NERVES_MAKE_NOSTRUCT(TestKitazonoBlockBrick, Stop)
}
TestKitazonoBlockBrick::TestKitazonoBlockBrick(const char* name) : al::LiveActor(name) {}
TestKitazonoBlockBrick::~TestKitazonoBlockBrick() {}
void TestKitazonoBlockBrick::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    if (al::isExistRail(this)) {
        al::setRailPosToStart(this);
        al::initNerve(this, &NrvTestKitazonoBlockBrick.Move, 0);
    } else {
        al::initNerve(this, &NrvTestKitazonoBlockBrick.Wait, 0);
    }
    mInitialPos = al::getTrans(this);
    mBreakModel = new TestKitazonoBlockBrickBreak("テスト北園壊れレンガブロック");
    mBreakModel->init(info);
    makeActorAppeared();
}
void TestKitazonoBlockBrick::control() {}
void TestKitazonoBlockBrick::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
void TestKitazonoBlockBrick::exeBreak() {
    mBreakModel->breakBlock(al::getTrans(this));
    kill();
}
void TestKitazonoBlockBrick::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) {
        if (al::isExistRail(this)) al::setNerve(this, &NrvTestKitazonoBlockBrick.Move);
        else al::setNerve(this, &NrvTestKitazonoBlockBrick.Wait);
    }
}
void TestKitazonoBlockBrick::exeMove() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    al::moveSyncRailTurn(this, 2.0f);
    if (al::isRailReachedEnd(this) || al::isRailReachedStart(this))
        al::setNerve(this, &NrvTestKitazonoBlockBrickStop);
}
void TestKitazonoBlockBrick::exeStop() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (al::isStep(this, 60)) al::setNerve(this, &NrvTestKitazonoBlockBrick.Move);
}
bool TestKitazonoBlockBrick::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (al::isNerve(this, &NrvTestKitazonoBlockBrick.Reaction)) return false;
    if (al::isMsgPlayerUpperPunch(msg)) {
        if (++mHitCount > 0) al::setNerve(this, &NrvTestKitazonoBlockBrick.Break);
        else al::setNerve(this, &NrvTestKitazonoBlockBrick.Reaction);
        return true;
    }
    return false;
}
